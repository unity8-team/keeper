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

#include <QDBusArgument>
#include <QObject>
#include <QScopedPointer>

typedef QMap<QString, QVariantMap> QVariantDictMap;

namespace keeper
{

class KeeperItem : public QVariantMap
{
public:
    KeeperItem();
    ~KeeperItem();
    explicit KeeperItem(QVariantMap const & values);

    // methods created for convenience
    bool has_property(QString const & property) const;
    QVariant get_property_value(QString const & property) const;

    // checks that the item is valid
    bool is_valid() const;

    // predefined properties
    QString get_type(bool *valid = nullptr) const;
    QString get_display_name(bool *valid = nullptr) const;
    QString get_dir_name(bool *valid = nullptr) const;
    QString get_status(bool *valid = nullptr) const;
    double get_percent_done(bool *valid = nullptr) const;
    keeper::KeeperError get_error(bool *valid = nullptr) const;

    // d-bus
    static void registerMetaType();
};

class KeeperItemsMap : public QMap<QString, KeeperItem>
{
public:
    KeeperItemsMap();
    ~KeeperItemsMap();
    explicit KeeperItemsMap(KeeperError error);

    QStringList get_uuids() const;

    keeper::KeeperError get_error() const;

    // d-bus
    static void registerMetaType();
    friend QDBusArgument &operator<<(QDBusArgument &argument, const KeeperItemsMap &result);
    friend const QDBusArgument &operator>>(const QDBusArgument &argument, KeeperItemsMap &result);

private:
    KeeperError error_ = KeeperError::OK;
};

} // namespace keeper

Q_DECLARE_METATYPE(keeper::KeeperItem)
Q_DECLARE_METATYPE(keeper::KeeperItemsMap)
