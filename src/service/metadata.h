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

#pragma once

#include <QString>
#include <QMap>
#include <QVariant>

class Metadata
{
public:

    Metadata();
    Metadata(const QString& key, const QString& display_name);
    ~Metadata();

    QString key() const { return key_; }
    QString display_name() const { return display_name_; }
    bool has_property(const QString& property_name) const;
    QVariant get_property(const QString& property_name) const;

    void set_property(const QString& property_name, const QVariant& value);

    QVariantMap get_public_properties() const;

private:

    QString key_;
    QString display_name_;
    QVariantMap properties_ {};
};
