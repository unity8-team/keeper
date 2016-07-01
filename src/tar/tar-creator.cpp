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
#include <QString>

class TarCreatorPrivate
{
public:

    TarCreatorPrivate(TarCreator* tar_creator, const QStringList& filenames, bool compress)
        : q_ptr(tar_creator)
        , filenames_(filenames)
        , compress_(compress)
    {
    }

    Q_DISABLE_COPY(TarCreatorPrivate)

    qint64 calculate_size()
    {
        return compress_ ? calculate_compressed_size() : calculate_uncompressed_size();
    }


    static int calculate_archive_open_callback(struct archive *, void *)
    {
        return ARCHIVE_OK;
    }
    static int calculate_archive_close_callback(struct archive *, void *)
    {
        return ARCHIVE_OK;
    }
    static ssize_t calculate_archive_write_callback(
        struct archive *,
        void * userdata,
        const void *,
        size_t len)
    {
        *static_cast<qint64*>(userdata) += len;
        return ssize_t(len);
    }

    static void add_file_header_to_archive(
        struct archive* archive,
        const QString& filename)
    {
        struct stat st;
        const auto filename_utf8 = filename.toUtf8();
        stat(filename_utf8.constData(), &st);
        auto entry = archive_entry_new();
        archive_entry_copy_stat(entry, &st);
        archive_entry_set_pathname(entry, filename_utf8.constData());
        if (archive_write_header(archive, entry) != ARCHIVE_OK)
            qCritical() << archive_error_string(archive);
        archive_entry_free(entry);
    }

    qint64 calculate_uncompressed_size()
    {
        qint64 archive_size {};

        auto a = archive_write_new();
        archive_write_set_format_pax(a);
        archive_write_open(a,
            &archive_size,
            calculate_archive_open_callback,
            calculate_archive_write_callback,
            calculate_archive_close_callback
        );

        for (const auto& filename : filenames_)
        {
            add_file_header_to_archive(a, filename);

            // don't bother with archive_write_data():
            // libarchive pads any missing data,
            // even if /all/ the data is missing
        }

        archive_write_close(a);
        archive_write_free(a);
        return archive_size;
    }

    qint64 calculate_compressed_size()
    {
        qint64 archive_size {};

        auto a = archive_write_new();
        archive_write_set_format_pax(a);
        archive_write_add_filter_xz(a);
        archive_write_open(a,
            &archive_size,
            calculate_archive_open_callback,
            calculate_archive_write_callback,
            calculate_archive_close_callback
        );

        for (const auto& filename : filenames_)
        {
            add_file_header_to_archive(a, filename);

            // write the content
            QFile file(filename);
            file.open(QIODevice::ReadOnly);
            char buf[4096];
            for(;;) {
                const auto n_read = file.read(buf, sizeof(buf));
                if (n_read == 0)
                    break;
                if (n_read > 0)
                    archive_write_data(a, buf, n_read);
            }
        }

        archive_write_close(a);
        archive_write_free(a);
        return archive_size;
    }

private:

    TarCreator * const q_ptr {};
    const QStringList filenames_ {};
    const bool compress_ {};
};

/**
***
**/

TarCreator::TarCreator(const QStringList& filenames, bool compress, QObject* parent)
    : QObject(parent)
    , d_ptr(new TarCreatorPrivate(this, filenames, compress))
{
}

TarCreator::~TarCreator() =default;

qint64
TarCreator::calculate_size()
{
    Q_D(TarCreator);

    return d->calculate_size();
}
