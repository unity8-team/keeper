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
const QString Metadata::USER_FOLDER_VALUE = QStringLiteral("folder");
const QString Metadata::SYSTEM_DATA_VALUE = QStringLiteral("system-data");
const QString Metadata::FOLDER_SYSTEM_VALUE = QStringLiteral("folder-system");
const QString Metadata::APPLICATION_VALUE = QStringLiteral("application");

///
///

Metadata::Metadata()
    : key_()
    , display_name_()
    , properties_()
{
}

Metadata::Metadata(const QString& key, const QString& display_name)
    : key_(key)
    , display_name_(display_name)
    , properties_()
{
}

bool
Metadata::has_property(const QString& property_name) const
{
    return properties_.contains(property_name);
}

QVariant
Metadata::get_property(const QString& property_name) const
{
    QVariant ret;

    auto it = properties_.constFind(property_name);
    if (it != properties_.end())
        ret = it.value();

    return ret;
}

void
Metadata::set_property(const QString& property_name, const QVariant& value)
{
    properties_.insert(property_name, value);
}

QVariantMap
Metadata::get_public_properties() const
{
    // they're all public so far...
    QVariantMap ret = properties_;
    ret.insert(QStringLiteral("key"), key_);
    ret.insert(QStringLiteral("display-name"), display_name_);
    return ret;
}
