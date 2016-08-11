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
 *     Xavi Garcia <xavi.garcia.mena@canonical.com>
 *     Charles Kerr <charles.kerr@canonical.com>
 */

#include "keeper-helper.h"
#include "keeper.h"

#include <QDebug>
#include <QDBusMessage>
#include <QDBusConnection>

KeeperHelper::KeeperHelper(Keeper* parent)
    : QObject(parent)
    , keeper_(*parent)
{
}

KeeperHelper::~KeeperHelper() = default;

QDBusUnixFileDescriptor KeeperHelper::StartBackup(quint64 n_bytes)
{
    // pass it back to Keeper to do the work
    Q_ASSERT(calledFromDBus());
    auto bus = connection();
    auto& msg = message();
    return keeper_.StartBackup(bus, msg, n_bytes);
}

QDBusUnixFileDescriptor KeeperHelper::StartRestore()
{
    // TODO get the file descriptor of the item in storage framework
    return QDBusUnixFileDescriptor();
}

void KeeperHelper::UpdateStatus(const QString &app_id, const QString &status, double percentage)
{
    qDebug() << "KeeperHelper::UpdateStatus(" << app_id << "," << status << "," << percentage << ")";
}
