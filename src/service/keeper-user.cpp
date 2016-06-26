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

#include <QDebug>
#include <QDBusMessage>
#include <QDBusConnection>

#include "service/metadata.h"
#include "service/keeper.h"
#include "service/keeper-user.h"

KeeperUser::KeeperUser(Keeper* keeper)
  : QObject(keeper)
  , keeper_(*keeper)
{
}

KeeperUser::~KeeperUser() =default;

QVariantMap
KeeperUser::GetPossibleBackups()
{
    QVariantMap ret;

    for (const auto& item : keeper_.GetPossibleBackups())
        ret[item.key()] = item.display_name();

    return ret;
}

void
KeeperUser::StartBackup (const QStringList& keys)
{
    // FIXME: writeme

    qInfo() << keys;
}

void
KeeperUser::Stop()
{
    // FIXME: writeme

    qInfo() << "hello world";
}

QVariantMap
KeeperUser::GetAvailableBackups()
{
    // FIXME: writeme

    QVariantMap ret;

    qInfo() << "hello world";

    return ret;
}

void
KeeperUser::StartRestore (const QStringList& keys)
{
    // FIXME: writeme

    qInfo() << keys;
}

QVariantDictMap
KeeperUser::getState() const
{
    // FIXME: writeme

    QVariantDictMap ret;

    qInfo() << "hello world";
    for (const auto& item : keeper_.GetPossibleBackups())
        ret[item.key()];

    return ret;
}
