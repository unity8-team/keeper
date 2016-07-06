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
 */

#pragma once

#include <QLocalSocket>
#include <QMap>
#include <QObject>
#include <QVector>

#include <memory>

class QTimer;

namespace ubuntu
{

namespace app_launch
{

class Registry;

} // namespace app_launch

} // namepsace ubuntu

namespace internal
{
class BackupHelperImpl : public QObject
{
    Q_OBJECT
public:

    BackupHelperImpl(QString const &appId, QObject * parent=nullptr);
    virtual ~BackupHelperImpl();
    Q_DISABLE_COPY(BackupHelperImpl)

    void init();
    void start(int sd);
    void stop();

    void emitHelperStarted();
    void emitHelperFinished();

    int get_helper_socket();

public Q_SLOTS:
    void inactivityDetected();
    void onSocketReadReady();

Q_SIGNALS:
    void started();
    void finished();

private:
    QString get_helper_path(QString const & appId);

    QString appid_;
    QTimer * timer_;
    std::shared_ptr<ubuntu::app_launch::Registry> registry_;
    QScopedPointer <QLocalSocket> storage_framework_socket_;
    QScopedPointer <QLocalSocket> helper_socket_;
    QScopedPointer <QLocalSocket> read_socket_;
};

} // namespace internal
