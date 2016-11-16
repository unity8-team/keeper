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
 *   Xavi Garcia Mena <xavi.garcia.mena@canonical.com>
 */
#pragma once

#include "util/connection-helper.h"

#include <QObject>
#include <QScopedPointer>
#include <QFile>

#include <memory>

class QLocalSocket;
class StorageFrameworkClient;
class Downloader;

class TestRestoreSocket : public QObject
{
    Q_OBJECT
public:
    explicit TestRestoreSocket(QObject *parent = nullptr);
    virtual ~TestRestoreSocket();

    void start(QString const & dir_name, QString const & file_name);

public Q_SLOTS:
    void read_data();
    void on_socket_received(std::shared_ptr<Downloader> const & downloader);
    void on_disconnected();
    void on_data_stored(qint64 n);

private:
    std::shared_ptr<QLocalSocket> socket_;
    QScopedPointer<StorageFrameworkClient> sf_client_;
    ConnectionHelper connections_;
    qint64 n_read_ = 0;
    QFile file_;
    QFutureWatcher<std::shared_ptr<Downloader>> future_watcher_;
};
