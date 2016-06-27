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
#include <QLocalSocket>
#include "storage_framework_client.h"


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

using namespace unity::storage::qt::client;

StorageFrameworkClient::StorageFrameworkClient(QObject *parent)
    : QObject(parent)
    , runtime_(Runtime::create())
    , close_uploader_watcher_(parent)
    , uploader_ready_watcher_(parent)
    , uploader_()
{
    QObject::connect(&uploader_ready_watcher_,&QFutureWatcher<std::shared_ptr<Uploader>>::finished, this, &StorageFrameworkClient::uploaderReady);
    QObject::connect(&close_uploader_watcher_,&QFutureWatcher<unity::storage::TransferState>::finished, this, &StorageFrameworkClient::uploaderClosed);
}


void StorageFrameworkClient::getNewFileForBackup()
{
    // Get the acccounts. (There is only one account for the local client implementation.)
    // We do this synchronously for simplicity.
    auto accounts = runtime_->accounts().result();
    Root::SPtr root = accounts[0]->roots().result()[0];
    qDebug() << "id:" << root->native_identity();
    qDebug() << "time:" << root->last_modified_time();


    // XGM ADD A new file to the root
    QFutureWatcher<std::shared_ptr<Uploader>> new_file_watcher;

    // get the current date and time to create the new file
    QDateTime now = QDateTime::currentDateTime();
    QString new_file_name = QString("Backup_%1").arg(now.toString("dd.MM.yyyy-hh:mm:ss.zzz"));

    uploader_ready_watcher_.setFuture(root->create_file(new_file_name));
}

int StorageFrameworkClient::getUploaderSocketDescriptor()
{
    if (!uploader_)
    {
        return -1;
    }
    auto socket = uploader_->socket();
    return int(socket->socketDescriptor());
}

void StorageFrameworkClient::closeUploader()
{
//    close(uploader_->socket()->socketDescriptor());
    uploader_->socket()->disconnectFromServer();
    uploader_->finish_upload();
}

void StorageFrameworkClient::uploaderReady()
{
    qDebug() << "StorageFrameworkClient::uploaderReady()";
    uploader_ = uploader_ready_watcher_.result();

    auto socket = uploader_->socket();
    auto sd = int(socket->socketDescriptor());
    Q_EMIT (socketReady(sd));

}

void StorageFrameworkClient::uploaderClosed()
{
    qDebug() << "StorageFrameworkClient::uploaderClosed()";
    auto state = close_uploader_watcher_.result();
    qDebug() << "StorageFrameworkClient finished with result: " << static_cast<int>(state);
    Q_EMIT (socketClosed());
}
