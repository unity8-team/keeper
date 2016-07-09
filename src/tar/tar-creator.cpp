/*
 * Copyright (C) 2016 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canoincal.com>
 */

#include "tar/tar-creator.h"

#include <archive.h>
#include <archive_entry.h>

#include <QDebug>
#include <QFile>
#include <QSharedPointer>
#include <QString>

#include <memory>

class TarCreator::Impl
{
public:

    Impl(const QStringList& filenames, bool compress)
        : filenames_(filenames)
        , compress_(compress)
        , step_archive_()
        , step_filenum_(-1)
        , step_file_()
        , step_buf_()
    {
    }

    ssize_t calculate_size() const
    {
        return compress_ ? calculate_compressed_size() : calculate_uncompressed_size();
    }

    bool step(std::vector<char>& fillme)
    {
        step_buf_.resize(0);
        bool success = true;

        // if this is the first step, create an archive
        if (!step_archive_)
        {
            step_archive_.reset(archive_write_new(), [](struct archive* a){archive_write_free(a);});
            archive_write_set_format_pax(step_archive_.get());
            if (compress_)
                archive_write_add_filter_xz(step_archive_.get());
            archive_write_open(step_archive_.get(), &step_buf_, nullptr, append_bytes_write_cb, nullptr);

            step_file_.reset();
            step_filenum_ = -1;
        }

        for(;;)
        {
            // if we don't have a file we're working on, then get one
            if (!step_file_)
            {
                if (step_filenum_ >= filenames_.size()) // tried to read past the end
                {
                    success = false;
                    break;
                }

                // step to next file
                if (++step_filenum_ == filenames_.size()) // we made it to the end!
                {
                    archive_write_close(step_archive_.get());
                    break;
                }

                // write the file's header
                const auto& filename = filenames_[step_filenum_];
                success = add_file_header_to_archive(step_archive_.get(), filename);
                if (!success)
                    break;

                // prep it for reading
                step_file_.reset(new QFile(filename));
                step_file_->open(QIODevice::ReadOnly);
            }

            static constexpr int BUFSIZE {1024*10};
            char inbuf[BUFSIZE];
            const auto n = step_file_->read(inbuf, sizeof(inbuf));
            if (n > 0) // got data
            {
                if (archive_write_data(step_archive_.get(), inbuf, size_t(n)) == -1)
                {
                    qWarning() << archive_error_string(step_archive_.get());
                    success = false;
                }
                break;
            }
            else if (n < 0) // read error
            {
                success = false;
                qWarning() << QStringLiteral("read()ing %1 returned %2 (%3)")
                                  .arg(step_file_->fileName())
                                  .arg(n)
                                  .arg(step_file_->errorString());

                break;
            }
            else if (step_file_->atEnd()) // eof
            {
                // loop to next file
                step_file_.reset();
                continue;
            }
        }

        std::swap(fillme,step_buf_);
        return success;
    }

private:

    static ssize_t append_bytes_write_cb(struct archive *,
                                         void * vtarget,
                                         const void * vsource,
                                         size_t len)
    {
        auto& target = *static_cast<std::vector<char>*>(vtarget);
        const auto& source = static_cast<const char*>(vsource);
        target.insert(target.end(), source, source+len);
        return ssize_t(len);
    }

    static ssize_t count_bytes_write_cb(struct archive *,
                                        void * userdata,
                                        const void *,
                                        size_t len)
    {
        *static_cast<ssize_t*>(userdata) += len;
        return ssize_t(len);
    }

    static bool add_file_header_to_archive(struct archive* archive,
                                           const QString& filename)
    {
        struct stat st;
        const auto filename_utf8 = filename.toUtf8();
        stat(filename_utf8.constData(), &st);

        auto entry = archive_entry_new();
        archive_entry_copy_stat(entry, &st);
        archive_entry_set_pathname(entry, filename_utf8.constData());

        int ret;
        do {
            ret = archive_write_header(archive, entry);
            if (ret==ARCHIVE_WARN)
                qWarning() << archive_error_string(archive);
            if (ret==ARCHIVE_FATAL)
                qFatal("%s", archive_error_string(archive));
        } while (ret == ARCHIVE_RETRY);

        archive_entry_free(entry);
        return (ret == ARCHIVE_OK) || (ret == ARCHIVE_WARN);
    }

    ssize_t calculate_uncompressed_size() const
    {
        ssize_t archive_size {};

        auto a = archive_write_new();
        archive_write_set_format_pax(a);
        archive_write_open(a, &archive_size, nullptr, count_bytes_write_cb, nullptr);

        for (const auto& filename : filenames_)
        {
            add_file_header_to_archive(a, filename);

            // libarchive pads any missing data,
            // so we don't need to call archive_write_data()
        }

        archive_write_close(a);
        archive_write_free(a);
        return archive_size;
    }

    ssize_t calculate_compressed_size() const
    {
        ssize_t archive_size {};

        auto a = archive_write_new();
        archive_write_set_format_pax(a);
        archive_write_add_filter_xz(a);
        archive_write_open(a, &archive_size, nullptr, count_bytes_write_cb, nullptr);

        for (const auto& filename : filenames_)
        {
            add_file_header_to_archive(a, filename);

            // process the file
            QFile file(filename);
            file.open(QIODevice::ReadOnly);
            static constexpr int BUFSIZE {4096};
            char buf[BUFSIZE];
            for(;;) {
                const auto n_read = file.read(buf, sizeof(buf));
                if (n_read == 0)
                    break;
                if (n_read > 0)
                    archive_write_data(a, buf, size_t(n_read));
                if (n_read < 0) {
                    qCritical() << QStringLiteral("Reading '%1' returned %2 (%3)")
                                      .arg(file.fileName())
                                      .arg(n_read)
                                      .arg(file.errorString());
                    break;
                }
            }
        }

        archive_write_close(a);
        archive_write_free(a);
        return archive_size;
    }

    const QStringList filenames_;
    const bool compress_ {};

    std::shared_ptr<struct archive> step_archive_;
    int step_filenum_ {-1};
    QSharedPointer<QFile> step_file_;
    std::vector<char> step_buf_;
};

/**
***
**/

TarCreator::TarCreator(const QStringList& filenames, bool compress)
    : impl_{new Impl{filenames, compress}}
{
}

TarCreator::~TarCreator() =default;

ssize_t
TarCreator::calculate_size() const
{
    return impl_->calculate_size();
}

bool
TarCreator::step(std::vector<char>& fillme)
{
    return impl_->step(fillme);
}
