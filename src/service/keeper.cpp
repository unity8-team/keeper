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
 * Author: Xavi Garcia <xavi.garcia.mena@canonical.com>
 */
#include <QDebug>
#include <QDBusMessage>
#include <QDBusConnection>

#include <helper/backup-helper.h>
#include "keeper.h"

namespace
{
    constexpr char const DEKKO_APP_ID[] = "dekko.dekkoproject_dekko_0.6.20";
}

Keeper::Keeper(QObject* parent)
    : QObject(parent),
      backup_helper_(new BackupHelper(DEKKO_APP_ID))
{
}

Keeper::~Keeper() = default;


void Keeper::start()
{
    qDebug() << "Backup start";

    // TODO
    // Here should go the code to retrieve the socket descriptor from storage framework
    int socket = 1999;
    backup_helper_->start(socket);
}
