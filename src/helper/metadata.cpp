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

#include "helper/metadata.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>

///
///

// JSON Keys
namespace
{
    constexpr const char PROPERTIES_KEY[]   = "properties";
}

///
///

Metadata::Metadata()
    : keeper::Item()
{
}

Metadata::Metadata(QJsonObject const & json)
    : keeper::Item()
{
    auto properties = json[PROPERTIES_KEY].toObject();
    for (auto const & key : properties.keys())
    {
        this->insert(key, properties[key].toString());
    }
}

Metadata::Metadata(QString const& uuid, QString const& display_name)
    : keeper::Item()
{
    this->insert(keeper::Item::UUID_KEY, uuid);
    this->insert(keeper::Item::DISPLAY_NAME_KEY, display_name);
}

QJsonObject
Metadata::json() const
{
    QJsonArray json_properties;
    QJsonObject properties_obj;
    for (auto iter = this->begin(); iter != this->end(); ++iter)
    {
        properties_obj[iter.key()] = (*iter).toString();
    }

    QJsonObject ret
    {
        { PROPERTIES_KEY, properties_obj }
    };

    return ret;
}
