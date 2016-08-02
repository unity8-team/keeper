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
#include <QVariant>

/**
 * Information about a backup
 */
class Metadata
{
public:

    Metadata();
    Metadata(const QString& key, const QString& display_name);

    // metadata keys
    static const QString TYPE_KEY;
    static const QString PATH_KEY;
    static const QString ICON_KEY;
    static const QString NAME_KEY;
    static const QString PACKAGE_KEY;
    static const QString TITLE_KEY;
    static const QString VERSION_KEY;

    // metadata values
    static const QString USER_FOLDER_VALUE;
    static const QString SYSTEM_DATA_VALUE;
    static const QString FOLDER_SYSTEM_VALUE;
    static const QString APPLICATION_VALUE;

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
