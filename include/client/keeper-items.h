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
 *     Xavi Garcia Mena <xavi.garcia.mena@canonical.com>
 */

#pragma once

#include "client/keeper-errors.h"

#include <QJsonObject>

typedef QMap<QString, QVariantMap> QVariantDictMap;

namespace keeper
{

class Item : public QVariantMap
{
public:
    Item();
    ~Item();
    explicit Item(QVariantMap const & values);

    // keys
    static QString const UUID_KEY;
    static QString const TYPE_KEY;
    static QString const SUBTYPE_KEY;
    static QString const NAME_KEY;
    static QString const PACKAGE_KEY;
    static QString const TITLE_KEY;
    static QString const VERSION_KEY;
    static QString const FILE_NAME_KEY;
    static QString const DIR_NAME_KEY;
    static QString const DISPLAY_NAME_KEY;
    static QString const STATUS_KEY;
    static QString const ERROR_KEY;
    static QString const PERCENT_DONE_KEY;
    static QString const SPEED_KEY;

    // values
    static QString const FOLDER_VALUE;
    static QString const SYSTEM_DATA_VALUE;
    static QString const APPLICATION_VALUE;


    // methods created for convenience
    bool has_property(QString const & property) const;
    template<typename T> T get_property(QString const & property, bool * valid) const;
    QVariant get_property_value(QString const & property) const;
    void set_property_value(QString const& property, QVariant const& value);

    // checks that the item is valid
    bool is_valid() const;

    // predefined properties
    QString get_uuid(bool *valid = nullptr) const;
    QString get_type(bool *valid = nullptr) const;
    QString get_display_name(bool *valid = nullptr) const;
    QString get_dir_name(bool *valid = nullptr) const;
    QString get_status(bool *valid = nullptr) const;
    double get_percent_done(bool *valid = nullptr) const;
    keeper::Error get_error(bool *valid = nullptr) const;
    QString get_file_name(bool *valid = nullptr) const;

    // d-bus
    static void registerMetaType();
};

class Items : public QMap<QString, Item>
{
public:
    Items();
    ~Items();
    explicit Items(Error error);

    QStringList get_uuids() const;

    // d-bus
    static void registerMetaType();

private:
    Error error_ = Error::OK;
};

} // namespace keeper

Q_DECLARE_METATYPE(keeper::Item)
Q_DECLARE_METATYPE(keeper::Items)
