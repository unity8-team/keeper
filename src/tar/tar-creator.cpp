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

    qint64 calculate_compressed_size()
    {
        return 0;
    }

    qint64 calculate_uncompressed_size()
    {
        qint64 archive_size {};

        auto a = archive_write_new();
        archive_write_set_format_pax(a);
        archive_write_open(a,
            &archive_size,
            [](struct archive*, void*){ return ARCHIVE_OK; }, // open
            [](struct archive*, void* userdata, const void*, size_t len){ // write
                  *static_cast<qint64*>(userdata) += len;
                  return ssize_t(len);
            },
            [](struct archive*, void*){ return ARCHIVE_OK; } // close
        );

        for (const auto& filename : filenames_)
        {
            const auto filename_utf8 = filename.toUtf8();
            struct stat st;
            stat(filename_utf8.constData(), &st);

            auto entry = archive_entry_new();
            archive_entry_copy_stat(entry, &st);
            archive_entry_set_pathname(entry, filename_utf8.constData());
            if (archive_write_header(a, entry) != ARCHIVE_OK)
                qCritical() << archive_error_string(a);
            archive_entry_free(entry);
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
