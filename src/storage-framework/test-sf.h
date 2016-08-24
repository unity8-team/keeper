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
#include <QLocalSocket>

#include <memory>

class QTimer;
class StorageFrameworkClient;

class TestSF : public QObject
{
    Q_OBJECT
public:
    TestSF(bool commit, QObject *parent = nullptr);
    virtual ~TestSF();

    Q_DISABLE_COPY(TestSF)

    void start();
    void write();
    void finish();
    void timeout_reached();

public Q_SLOTS:
    void sf_socket_ready(std::shared_ptr<QLocalSocket> const& sf_socket);
    void on_sf_socket_state_changed(QLocalSocket::LocalSocketState socketState);
    void on_sf_socket_finished();

private:
    enum class State { INACTIVE, WAITING_SOCKET, SOCKET_READY, WROTE, WAITING_FINISH };
    QScopedPointer<QTimer> timer_;
    QScopedPointer<StorageFrameworkClient> sf_;
    std::shared_ptr<QLocalSocket> sf_socket_;
    bool commit_;
    State state_;
    
};
