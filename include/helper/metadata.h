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

#include <QMap>
#include <QString>

/**
 * Information about a backup
 */
class Metadata
{
public:

    Metadata();
    Metadata(QString const& uuid, QString const& display_name);

    // metadata keys
    static QString const TYPE_KEY;
    static QString const SUBTYPE_KEY;
    static QString const NAME_KEY;
    static QString const PACKAGE_KEY;
    static QString const TITLE_KEY;
    static QString const VERSION_KEY;

    // metadata values
    static QString const FOLDER_VALUE;
    static QString const SYSTEM_DATA_VALUE;
    static QString const APPLICATION_VALUE;

    QString uuid() const { return uuid_; }
    QString display_name() const { return display_name_; }
    bool get_property(QString const& property_name, QString& setme_value) const;
    void set_property(QString const& property_name, QString const& value);

    QMap<QString,QString> get_public_properties() const;

private:

    QString uuid_;
    QString display_name_;
    QMap<QString,QString> properties_{};
};
