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

#pragma once

#include "client/keeper-items.h"
#include <QJsonObject>
#include <QMap>
#include <QString>

/**
 * Information about a backup or restore item
 */
class Metadata : public keeper::Item
{
public:

    Metadata();
    explicit Metadata(QJsonObject const & json_object);
    Metadata(QString const& uuid, QString const& display_name);

    QJsonObject json() const;
};
