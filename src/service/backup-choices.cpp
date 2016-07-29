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

#include <uuid/uuid.h>

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
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

BackupChoices::BackupChoices() =default;

BackupChoices::~BackupChoices() =default;

QVector<Metadata>
BackupChoices::get_backups()
{
    QVector<Metadata> ret;

    //
    //  System Data
    //

    const auto type_key = QStringLiteral("type");
    const auto system_data_str = QStringLiteral("system-data");
    {
        Metadata m(generate_new_uuid(), "System Data"); // FIXME: how to i18n in a Qt DBus service?
        m.set_property(type_key, system_data_str);
        ret.push_back(m);
    }

    //
    //  Click Packages
    //

    QString manifests_str;
    GError* error {};
    auto user = click_user_new_for_user(nullptr, nullptr, &error);
    if (user != nullptr)
    {
        auto tmp = click_user_get_manifests_as_string (user, &error);
        manifests_str = QString::fromUtf8(tmp);
        g_clear_pointer(&tmp, g_free);
        g_clear_object(&user);
    }
    if (error != nullptr)
    {
        qCritical() << "Error getting click manifests: " << error->message;
        g_clear_error(&error);
    }

    const auto name_key = QStringLiteral("name");
    const auto package_key = QStringLiteral("package");
    const auto title_key = QStringLiteral("title");
    const auto version_key = QStringLiteral("version");
    const auto application_str = QStringLiteral("application");

    auto loadDoc = QJsonDocument::fromJson(manifests_str.toUtf8());
    auto tmp = loadDoc.toJson();
    if (loadDoc.isArray())
    {
        auto manifests = loadDoc.array();
        for(auto it=manifests.begin(), end=manifests.end(); it!=end; ++it)
        {
            const auto& manifest (*it);
            if (manifest.isObject())
            {
                auto o = manifest.toObject();

                // manditory name
                const auto name = o[name_key];
                if (name == QJsonValue::Undefined)
                    continue;

                // manditory title
                const auto title = o[title_key];
                if (title == QJsonValue::Undefined)
                    continue;
                QString display_name = title.toString();

                // if version is available, append it to display_name
                const auto version = o[version_key];
                if (version != QJsonValue::Undefined)
                    display_name = QStringLiteral("%1 (%2)").arg(display_name).arg(version.toString());

                Metadata m(generate_new_uuid(), display_name);
                m.set_property(package_key, name.toString());
                m.set_property(type_key, application_str);

                if (version != QJsonValue::Undefined)
                    m.set_property(version_key, version.toString());

                ret.push_back(m);
            }
        }
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

    const auto path_key = QStringLiteral("path");
    const auto user_folder_str = QStringLiteral("folder");
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
            m.set_property(path_key, locations.front());
            m.set_property(type_key, user_folder_str);
            ret.push_back(m);
        }
    }

    return ret;
}
