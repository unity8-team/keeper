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
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#include "service/backup-choices.h"

#include <uuid/uuid.h>

#include <QDebug>
#include <QStandardPaths>
#include <QString>

#include <array>

namespace
{
    QString generate_new_uuid()
    {
        uuid_t keyuu;
        uuid_generate(keyuu);
        char keybuf[37];
        uuid_unparse(keyuu, keybuf);
        return QString::fromUtf8(keybuf);
    }
}

BackupChoices::BackupChoices(QObject *parent)
    : MetadataProvider(parent)
{
}

BackupChoices::~BackupChoices() =default;

QVector<Metadata>
BackupChoices::get_backups() const
{
    return backups_;
}

void
BackupChoices::get_backups_async()
{
    backups_.clear();
    //
    //  System Data
    //
    {
        Metadata m(generate_new_uuid(), "System Data"); // FIXME: how to i18n in a Qt DBus service?
        m.set_property_value(Metadata::TYPE_KEY, Metadata::SYSTEM_DATA_VALUE);
        backups_.push_back(m);
    }

    //
    //  XDG User Directories
    //

    const std::array<QStandardPaths::StandardLocation,4> standard_locations = {
        QStandardPaths::DocumentsLocation,
        QStandardPaths::MoviesLocation,
        QStandardPaths::PicturesLocation,
        QStandardPaths::MusicLocation
    };

    for (const auto& location : standard_locations)
    {
        const auto name = QStandardPaths::displayName(location);
        const auto locations = QStandardPaths::standardLocations(location);
        if (locations.empty())
        {
            qWarning() << "unable to find path for"  << name;
        }
        else
        {
            const auto keystr = generate_new_uuid();
            Metadata m(keystr, name);
            m.set_property_value(Metadata::TYPE_KEY, Metadata::FOLDER_VALUE);
            m.set_property_value(Metadata::SUBTYPE_KEY, locations.front());
            backups_.push_back(m);
        }
    }

    Q_EMIT(finished(keeper::Error::OK));
}
