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

#include "service/metadata.h"

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

Metadata::~Metadata() =default;
    
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
