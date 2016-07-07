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

#include "backup-helper-impl.h"

#include <QTimer>
#include <QDebug>

#include <ubuntu-app-launch/registry.h>
#include <service/app-const.h>

extern "C" {
    #include <ubuntu-app-launch.h>
}

#include <fcntl.h>
#include <sys/socket.h>

typedef void* gpointer;
typedef char gchar;

namespace
{
    const int MAX_TIME_WAITING_FOR_DATA = 10000;
}

static void helper_observer_cb(const gchar* appid, const gchar* /*instance*/, const gchar* /*type*/, gpointer user_data)
{
    qDebug() << "HELPER STARTED +++++++++++++++++++++++++++++++++++++ " << appid;
    internal::BackupHelperImpl * obj = static_cast<internal::BackupHelperImpl*>(user_data);
    if (obj)
    {
        qDebug() << "I have a valid object";
        obj->emitHelperStarted();
    }
}

static void helper_observer_stop_cb(const gchar* appid, const gchar* /*instance*/, const gchar* /*type*/, gpointer user_data)
{
    qDebug() << "HELPER FINISHED +++++++++++++++++++++++++++++++++++++ " << appid;
    internal::BackupHelperImpl * obj = static_cast<internal::BackupHelperImpl*>(user_data);
    if (obj)
    {
        qDebug() << "I have a valid object";
        obj->emitHelperFinished();
    }
}

namespace internal
{

BackupHelperImpl::BackupHelperImpl(QString const & appid, QObject * parent) :
         QObject(parent)
        ,appid_(appid)
        ,timer_(new QTimer(this))
        ,registry_(new ubuntu::app_launch::Registry())
        ,storage_framework_socket_(new QLocalSocket())
        ,helper_socket_(new QLocalSocket())
        ,read_socket_(new QLocalSocket())
{
    init();
}

BackupHelperImpl::~BackupHelperImpl()
{
    // Remove the observers
    ubuntu_app_launch_observer_delete_helper_started(helper_observer_cb, HELPER_TYPE, this);
    ubuntu_app_launch_observer_delete_helper_stop(helper_observer_stop_cb, HELPER_TYPE, this);

}

void BackupHelperImpl::init()
{
    connect(timer_, &QTimer::timeout, this, &BackupHelperImpl::inactivityDetected);
    // install the start observer
    ubuntu_app_launch_observer_add_helper_started(helper_observer_cb, HELPER_TYPE, this);
    ubuntu_app_launch_observer_add_helper_stop(helper_observer_stop_cb, HELPER_TYPE, this);

    int fds[2];
    int rc = socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fds);
    if (rc == -1)
    {
        // TODO throw exception.
        qWarning() << "BackupHelperImpl::init(): error creating socket pair to communicate with helper ";
        return;
    }

    // helper socket is for the client.
    helper_socket_->setSocketDescriptor(fds[1], QLocalSocket::ConnectedState, QIODevice::WriteOnly);

    read_socket_->setSocketDescriptor(fds[0], QLocalSocket::ConnectedState, QIODevice::ReadOnly);
    connect(read_socket_.data(), &QLocalSocket::readyRead, this, &BackupHelperImpl::onSocketReadReady);
}

void BackupHelperImpl::start(int sd)
{
    qDebug() << "Starting helper for app: " << appid_;

    auto backupType = ubuntu::app_launch::Helper::Type::from_raw(HELPER_TYPE);

    auto appid = ubuntu::app_launch::AppID::parse(appid_.toStdString());
    auto helper = ubuntu::app_launch::Helper::create(backupType, appid, registry_);

    std::vector<ubuntu::app_launch::Helper::URL> urls = {
        ubuntu::app_launch::Helper::URL::from_raw(get_helper_path(appid_).toStdString())
    };

    storage_framework_socket_->setSocketDescriptor(sd, QLocalSocket::ConnectedState, QIODevice::WriteOnly);
    timer_->start(MAX_TIME_WAITING_FOR_DATA);
    helper->launch(urls);
}

void BackupHelperImpl::stop()
{
    qDebug() << "Stopping helper for app: " << appid_;
    auto backupType = ubuntu::app_launch::Helper::Type::from_raw(HELPER_TYPE);

    auto appid = ubuntu::app_launch::AppID::parse(appid_.toStdString());
    auto helper = ubuntu::app_launch::Helper::create(backupType, appid, registry_);

    auto instances = helper->instances();

    if (instances.size() > 0 )
    {
        qDebug() << "We have instances";
        instances[0]->stop();
    }
}

void BackupHelperImpl::inactivityDetected()
{
    qWarning() << "Inactivity detected in the helper...stopping it";
    this->stop();
}

void BackupHelperImpl::emitHelperStarted()
{
    qDebug() << "BACKUP STARTED SIGNAL";
    Q_EMIT started();
}

void BackupHelperImpl::emitHelperFinished()
{
    qDebug() << "BACKUP FINISHED SIGNAL";
    Q_EMIT finished();
}

QString BackupHelperImpl::get_helper_path(QString const & /*appId*/)
{
    //TODO retrieve the helper path from the package information

    // This is added for testing purposes only
    auto testHelper = qgetenv("KEEPER_TEST_HELPER");
    if (testHelper != "")
    {
        qDebug() << "BackupHelperImpl::getHelperPath: returning the helper: " << testHelper;
        return testHelper;
    }

    return DEKKO_HELPER_BIN;
}

void BackupHelperImpl::onSocketReadReady()
{
    timer_->stop();
    qDebug() << "BackupHelperImpl::onSocketReadReady() " << read_socket_->socketDescriptor();
    auto buf = read_socket_->read(read_socket_->bytesAvailable());
    if (buf.size() != 0)
    {
        auto bytes_written = storage_framework_socket_->write(buf);
        if (bytes_written != buf.size())
        {
            // TODO: Store error details.
            qWarning() << "Error writing to the storage framework socket: " << storage_framework_socket_->errorString();
            return;
        }
    }
    timer_->start(MAX_TIME_WAITING_FOR_DATA);
}

int BackupHelperImpl::get_helper_socket()
{
    return static_cast<int>(helper_socket_->socketDescriptor());
}

} // namespace internal
