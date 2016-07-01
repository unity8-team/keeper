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

#include "service/metadata.h"
#include "service/keeper.h"
#include "service/keeper-user.h"

#include <QDebug>
#include <QDBusMessage>
#include <QDBusConnection>

KeeperUser::KeeperUser(Keeper* keeper)
  : QObject(keeper)
  , keeper_(*keeper)
{
}

KeeperUser::~KeeperUser() =default;

QVariantDictMap
KeeperUser::GetBackupChoices()
{
    QVariantDictMap ret;
    for (const auto& metadata : keeper_.get_backup_choices())
        ret.insert(metadata.key(), metadata.get_public_properties());
    return ret;
}

void
KeeperUser::StartBackup (const QStringList& keys)
{
    // FIXME: writeme

    qDebug() << keys;
}

void
KeeperUser::Cancel()
{
    // FIXME: writeme

    qDebug() << "hello world";
}

QVariantDictMap
KeeperUser::GetRestoreChoices()
{
    QVariantDictMap ret;
    for (const auto& metadata : keeper_.get_restore_choices())
        ret.insert(metadata.key(), metadata.get_public_properties());
    return ret;
}

void
KeeperUser::StartRestore (const QStringList& keys)
{
    // FIXME: writeme

    qDebug() << keys;
}

QVariantDictMap
KeeperUser::get_state() const
{
    // FIXME: writeme (the code below is junk 'hello world' data for testing in d-feet)

    QVariantDictMap ret;
    for (const auto& item : keeper_.get_backup_choices())
        ret[item.key()];
    return ret;
}
