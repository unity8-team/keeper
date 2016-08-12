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
#pragma once

#include <QObject>
#include <QFutureWatcher>

#include <unity/storage/qt/client/client-api.h>

class StorageFrameworkClient : public QObject
{
    Q_OBJECT
public:
    Q_DISABLE_COPY(StorageFrameworkClient)
    StorageFrameworkClient(QObject *parent = nullptr);
    virtual ~StorageFrameworkClient() = default;

    // TODO this is a simple method to obtain a new file for a backup
    // we'll need to get it from a UI, but the result should be the same
    // As the storage framework works async, we'll receive the Uploader in a signal
    // coming from a QFutureWatcher
    void getNewFileForBackup(quint64 n_bytes);

    void finish(bool do_commit);

Q_SIGNALS:
    void socketReady(std::shared_ptr<QLocalSocket> const& sf_socket);
    void finished();

private Q_SLOTS:
    void uploaderReady();
    void onUploaderClosed();

private:
    unity::storage::qt::client::Runtime::SPtr runtime_;

    // watchers
    QFutureWatcher<std::shared_ptr<unity::storage::qt::client::Uploader>> uploader_ready_watcher_;
    QFutureWatcher<std::shared_ptr<unity::storage::qt::client::File>> uploader_closed_watcher_;
    std::shared_ptr<unity::storage::qt::client::Uploader> uploader_;
};
