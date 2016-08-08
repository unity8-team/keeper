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

#include "helper/metadata.h"

///
///

// Metadata keys
const QString Metadata::TYPE_KEY = QStringLiteral("type");
const QString Metadata::SUBTYPE_KEY = QStringLiteral("subtype");
const QString Metadata::PATH_KEY = QStringLiteral("path");
const QString Metadata::NAME_KEY = QStringLiteral("name");
const QString Metadata::PACKAGE_KEY = QStringLiteral("package");
const QString Metadata::TITLE_KEY = QStringLiteral("title");
const QString Metadata::VERSION_KEY = QStringLiteral("version");

// Metadata values
const QString Metadata::FOLDER_VALUE = QStringLiteral("folder");
const QString Metadata::SYSTEM_DATA_VALUE = QStringLiteral("system-data");
const QString Metadata::APPLICATION_VALUE = QStringLiteral("application");

///
///

Metadata::Metadata()
    : uuid_()
    , display_name_()
    , properties_()
{
}

Metadata::Metadata(QString const& uuid, QString const& display_name)
    : uuid_(uuid)
    , display_name_(display_name)
    , properties_()
{
}

bool
Metadata::get_property(QString const& property_name, QString& setme) const
{
    auto it = properties_.constFind(property_name);
    const bool found = it != properties_.end();

    if (found)
        setme = it.value();

    return found;
}

void
Metadata::set_property(QString const& property_name, QString const& value)
{
    properties_.insert(property_name, value);
}

QMap<QString,QString>
Metadata::get_public_properties() const
{
    // they're all public so far...
    auto ret = properties_;
    ret.insert(QStringLiteral("uuid"), uuid_);
    ret.insert(QStringLiteral("display-name"), display_name_);
    return ret;
}
