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
#include "backup-helper-impl.h"

#include <QTimer>
#include <QDebug>

#include <ubuntu-app-launch/registry.h>

extern "C" {
    #include <ubuntu-app-launch.h>
}

typedef void* gpointer;
typedef char gchar;

namespace
{
constexpr char const HELPER_TYPE[] = "backup-helper";

// TODO change this to something not fixed
constexpr char const HELPER_BIN[] = "/custom/click/dekko.dekkoproject/0.6.20/localclient";
}

static void helper_observer_cb(const gchar* appid, const gchar* instance, const gchar* type, gpointer user_data)
{
    qDebug() << "HELPER STARTED +++++++++++++++++++++++++++++++++++++ " << appid;
    internal::BackupHelperImpl * obj = static_cast<internal::BackupHelperImpl*>(user_data);
    if (obj)
    {
        qDebug() << "I have a valid object";
        obj->emitHelperStarted();
    }
}

static void helper_observer_stop_cb(const gchar* appid, const gchar* instance, const gchar* type, gpointer user_data)
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
    connect(timer_, &QTimer::timeout, this, &BackupHelperImpl::checkStatus);
    // install the start observer
    ubuntu_app_launch_observer_add_helper_started(helper_observer_cb, HELPER_TYPE, this);
    ubuntu_app_launch_observer_add_helper_stop(helper_observer_stop_cb, HELPER_TYPE, this);
}

void BackupHelperImpl::start(int socket)
{
    qDebug() << "Starting helper for app: " << appid_ << " and socket: " << socket;

    auto backupType = ubuntu::app_launch::Helper::Type::from_raw(HELPER_TYPE);

    auto appid = ubuntu::app_launch::AppID::parse(appid_.toStdString());
    auto helper = ubuntu::app_launch::Helper::create(backupType, appid, registry_);

    // TODO in here we should search for the helper bin and pass it
    // instead of setting a fixed one.
    std::vector<ubuntu::app_launch::Helper::URL> urls = {
        ubuntu::app_launch::Helper::URL::from_raw(HELPER_BIN),
        ubuntu::app_launch::Helper::URL::from_raw("1999")};

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

void BackupHelperImpl::checkStatus()
{
    // TODO due to the errors we get with apparmor I've removed this code
    // it simply checks with a timer that the socket coming from the
    // storage framework is receiving data.
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

} // namespace internal
