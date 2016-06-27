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

#include "service/backup-choices.h"

#include <click.h>

#include <QDebug>
#include <QStandardPaths>
#include <QString>

#include <array>

#include <uuid/uuid.h>

BackupChoices::BackupChoices() =default;

BackupChoices::~BackupChoices() =default;

QVector<Metadata>
BackupChoices::get_backups()
{
    QVector<Metadata> ret;

    // Click Packages

    GError* error {};
    auto user = click_user_new_for_user(nullptr, nullptr, &error);
    if (user != nullptr)
    {
        auto manifests = click_user_get_manifests_as_string (user, &error);
        g_message("manifests: %s", manifests);
        g_clear_pointer(&manifests, g_free);
        g_clear_object(&user);
    }


    // XDG User Directories

    const std::array<QStandardPaths::StandardLocation,4> standard_locations = {
        QStandardPaths::DocumentsLocation,
        QStandardPaths::MoviesLocation,
        QStandardPaths::PicturesLocation,
        QStandardPaths::MusicLocation
    };

    const auto path_str = QString::fromUtf8("path");

    for (const auto& sl : standard_locations)
    {
        const auto name = QStandardPaths::displayName(sl);
        const auto locations = QStandardPaths::standardLocations(sl);
        if (locations.empty())
        {
            qWarning() << "unable to find path for"  << name;
        }
        else
        {
            uuid_t keyuu;
            uuid_generate(keyuu);
            char keybuf[37];
            uuid_unparse(keyuu, keybuf);
            const auto keystr = QString::fromUtf8(keybuf);

            Metadata m(keystr, name);
            m.set_property(path_str, locations.front());
            ret.push_back(m);
        }
    }

    return ret;
}
