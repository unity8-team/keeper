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

#include "helper/metadata.h"
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

keeper::Items
KeeperUser::GetBackupChoices()
{
    auto bus = connection();
    auto& msg = message();
    return keeper_.get_backup_choices_var_dict_map(bus, msg);
}

void
KeeperUser::StartBackup (const QStringList& keys)
{
    Q_ASSERT(calledFromDBus());

    auto bus = connection();
    auto& msg = message();
    keeper_.start_tasks(keys, bus, msg);
}

void
KeeperUser::Cancel()
{
    keeper_.cancel();
}

keeper::Items
KeeperUser::GetRestoreChoices()
{
    Q_ASSERT(calledFromDBus());

    auto bus = connection();
    auto& msg = message();
    return keeper_.get_restore_choices(bus, msg);
}

void
KeeperUser::StartRestore (const QStringList& keys)
{
    Q_ASSERT(calledFromDBus());

    auto bus = connection();
    auto& msg = message();
    // if we start a restore right after a backup the uuid
    // will be found as a backup uuid.
    // Just clear the backup cache to avoid that.
    keeper_.invalidate_choices_cache();
    keeper_.start_tasks(keys, bus, msg);
}

keeper::Items
KeeperUser::get_state() const
{
    return keeper_.get_state();
}
