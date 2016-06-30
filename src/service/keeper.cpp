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
#include <storage-framework/storage_framework_client.h>
#include "keeper.h"

namespace
{
    constexpr char const DEKKO_APP_ID[] = "dekko.dekkoproject_dekko_0.6.20";
}

Keeper::Keeper(QObject* parent)
    : QObject(parent),
      backup_helper_(new BackupHelper(DEKKO_APP_ID)),
      storage_(new StorageFrameworkClient(this))
{
    QObject::connect(storage_.data(), &StorageFrameworkClient::socketReady, this, &Keeper::socketReady);
    QObject::connect(storage_.data(), &StorageFrameworkClient::socketClosed, this, &Keeper::socketClosed);
    QObject::connect(backup_helper_.data(), &BackupHelper::started, this, &Keeper::helperStarted);
    QObject::connect(backup_helper_.data(), &BackupHelper::finished, this, &Keeper::helperFinished);
}

Keeper::~Keeper() = default;


void Keeper::start()
{
    qDebug() << "Backup start";
    qDebug() << "Waiting for a valid socket from the storage framework";

    storage_->getNewFileForBackup();
}

QDBusUnixFileDescriptor Keeper::GetBackupSocketDescriptor()
{
    qDebug() << "Sending the socket " << storage_->getUploaderSocketDescriptor();
    return QDBusUnixFileDescriptor(storage_->getUploaderSocketDescriptor());
}

// FOR TESTING PURPOSES ONLY
// we should finish when the helper finishes
void Keeper::finish()
{
    qDebug() << "Closing the socket-------";
    storage_->closeUploader();
}

void Keeper::socketReady(int sd)
{
    qDebug() << "I've got a new socket: " << sd;
    qDebug() << "Starting the backup helper";
    backup_helper_->start();
}

void Keeper::helperStarted()
{
    qDebug() << "Backup helper started";
}

void Keeper::helperFinished()
{
    qDebug() << "Backup helper finished";
    qDebug() << "Closing the socket";
    storage_->closeUploader();
}

void Keeper::socketClosed()
{
    qDebug() << "The storage framework socket was closed";
}
