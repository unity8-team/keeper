/*
 * Copyright (C) 2013 Canonical, Ltd.
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
 * Author: Pete Woods <pete.woods@canonical.com>
 */

#pragma once

#include <QDBusMetaType>
#include <QtCore>
#include <QString>
#include <QVariantMap>
#include <client/keeper-errors.h>
#include "client/keeper-items.h"

typedef QMap<QString, QVariantMap> QVariantDictMap;
Q_DECLARE_METATYPE(QVariantDictMap)

typedef QMap<QString, QString> QStringMap;
Q_DECLARE_METATYPE(QStringMap)

namespace DBusTypes
{
    inline void registerMetaTypes()
    {
        qRegisterMetaType<QVariantDictMap>("QVariantDictMap");
        qRegisterMetaType<QStringMap>("QStringMap");
        qRegisterMetaType<keeper::Error>("keeper::Error");

        qDBusRegisterMetaType<QVariantDictMap>();
        qDBusRegisterMetaType<QStringMap>();
        qDBusRegisterMetaType<keeper::Error>();

        keeper::KeeperItemsMap::registerMetaType();
    }

    constexpr const char KEEPER_SERVICE[] = "com.canonical.keeper";

    constexpr const char KEEPER_SERVICE_PATH[] = "/com/canonical/keeper";

    constexpr const char KEEPER_HELPER_INTERFACE[] = "com.canonical.keeper.Helper";

    constexpr const char KEEPER_HELPER_PATH[] = "/com/canonical/keeper/helper";

    constexpr const char KEEPER_USER_INTERFACE[] = "com.canonical.keeper.User";

    constexpr const char KEEPER_USER_PATH[]      = "/com/canonical/keeper/user";
}
