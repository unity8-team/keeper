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

namespace
{
    constexpr char const TMP_SUFFIX[] = ".KEEPER_TMP";
}


StorageFrameworkClient::StorageFrameworkClient(QObject *parent)
    : QObject(parent)
    , runtime_(Runtime::create())
    , uploader_ready_watcher_(parent)
    , uploader_closed_watcher_(parent)
    , uploader_()
{
    QObject::connect(&uploader_ready_watcher_,&QFutureWatcher<std::shared_ptr<Uploader>>::finished, this, &StorageFrameworkClient::uploaderReady);
    QObject::connect(&uploader_closed_watcher_,&QFutureWatcher<std::shared_ptr<unity::storage::qt::client::File>>::finished, this, &StorageFrameworkClient::onUploaderClosed);

}


void StorageFrameworkClient::getNewFileForBackup(quint64 n_bytes)
{
    // Get the acccounts. (There is only one account for the local client implementation.)
    // We do this synchronously for simplicity.
    try {
        auto accounts = runtime_->accounts().result();
        Root::SPtr root = accounts[0]->roots().result()[0];
        qDebug() << "id:" << root->native_identity();
        qDebug() << "time:" << root->last_modified_time();


        // XGM ADD A new file to the root
        QFutureWatcher<std::shared_ptr<Uploader>> new_file_watcher;

        // get the current date and time to create the new file
        QDateTime now = QDateTime::currentDateTime();
        QString new_file_name = QString("Backup_%1%2").arg(now.toString("dd.MM.yyyy-hh:mm:ss.zzz")).arg(TMP_SUFFIX);

        uploader_ready_watcher_.setFuture(root->create_file(new_file_name, n_bytes));
    }
    catch (std::exception & e)
    {
        qDebug() << "ERROR: StorageFrameworkClient::getNewFileForBackup():" << e.what();
    }
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

void StorageFrameworkClient::finish(bool do_commit)
{

    if (!uploader_ || !do_commit)
    {
        qDebug() << "StorageFrameworkClient::finish() is throwing away the file";
        uploader_.reset();
        Q_EMIT(finished());
    }
    else try
    {
        qDebug() << "StorageFrameworkClient::finish() is committing";
        uploader_closed_watcher_.setFuture(uploader_->finish_upload());
    }
    catch (std::exception & e)
    {
        qDebug() << "ERROR: StorageFrameworkClient::closeUploader():" << e.what();
    }
}

void StorageFrameworkClient::onUploaderClosed()
{
    auto file = uploader_closed_watcher_.result();
    qDebug() << "Uploader for file" << file->name() << "was closed";
    qDebug() << "Uploader was closed";
    uploader_->socket()->disconnectFromServer();
    uploader_.reset();
    Q_EMIT(finished());
}

void StorageFrameworkClient::uploaderReady()
{
    uploader_ = uploader_ready_watcher_.result();

    Q_EMIT (socketReady(uploader_->socket()));
}

bool StorageFrameworkClient::removeTmpSuffix(std::shared_ptr<unity::storage::qt::client::File> const &file)
{
    if (!file)
    {
        qWarning() << "The storage framework file passed to remove tmp suffix is null";
        return false;
    }
    try
    {
        qDebug() << "Retrieving parents of file" << file->name();

        // block here until we get a response
        QVector<std::shared_ptr<Folder>> parents = file->parents().result();

        QString suffix = QString(TMP_SUFFIX);
        QString newName = file->name();

        if (file->name().length() < suffix.length() + 1)
        {
            qWarning() << "The file" << file->name() << "has an invalid name, and could not remove the suffix" << TMP_SUFFIX << "from it";
            return false;
        }
        newName.remove((file->name().length() - suffix.length()), suffix.length());
        // rename the file for all its parents
        // Storage framework states that we should not assume that a single parent is returned.
        for (const auto& parent : parents)
        {
            auto item = file->move(parent, newName).result();
            qDebug() << "File name changed to" << item->name();
        }
    }
    catch (std::exception & e)
    {
        qDebug() << "ERROR: StorageFrameworkClient::removeTmpSuffix():" << e.what();
        return false;
    }
    return true;
}

bool StorageFrameworkClient::deleteFile(std::shared_ptr<unity::storage::qt::client::File> const &file)
{
    try
    {
        auto res = file->delete_item();
        res.waitForFinished();
    }
    catch (std::exception & e)
    {
        qDebug() << "ERROR: StorageFrameworkClient::deleteFile():" << e.what();
        return false;
    }
    return true;
}
