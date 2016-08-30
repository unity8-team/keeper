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
 *     Xavi Garcia Mena <xavi.garcia.mena@canonical.com>
 */

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QDBusMetaType>

#include "upstart-defs.h"

#include "UpstartMockAdaptor.h"
#include "upstart-mock.h"

#include "UpstartJobMockAdaptor.h"
#include "upstart-job-mock.h"

#include "UpstartInstanceMockAdaptor.h"

#include <string>


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.interface()->isServiceRegistered(UPSTART_SERVICE))
    {
        auto service = new testing::UpstartMock(&app);
        auto upstart_adaptor = QSharedPointer<UpstartMockAdaptor>(new UpstartMockAdaptor(service));

        if (!connection.registerService(UPSTART_SERVICE))
        {
            qFatal("Could not register Upstart service %s.", connection.lastError().message().toStdString().c_str());
        }

        if (!connection.registerObject(UPSTART_PATH, service))
        {
            qFatal("Could not register Upstart object. %s", connection.lastError().message().toStdString().c_str());
        }

        auto instance_service = new testing::UpstartInstanceMock(UPSTART_HELPER_INSTANCE_NAME,&app);
        new UpstartInstanceMockAdaptor(instance_service);
        if (!connection.registerObject(UPSTART_HELPER_INSTANCE_PATH, instance_service))
        {
            qFatal("Could not register Upstart instance. %s", connection.lastError().message().toStdString().c_str());
        }

        auto multi_instance_service = new testing::UpstartInstanceMock(UPSTART_HELPER_MULTI_INSTANCE_NAME,&app);
        new UpstartInstanceMockAdaptor(multi_instance_service);
        if (!connection.registerObject(UPSTART_HELPER_MULTI_INSTANCE_PATH, multi_instance_service))
        {
            qFatal("Could not register Upstart multi instance. %s", connection.lastError().message().toStdString().c_str());
        }

        auto job_service = new testing::UpstartJobMock(upstart_adaptor, &app);
        new UpstartJobMockAdaptor(job_service);
        if (!connection.registerObject(UPSTART_HELPER_JOB_PATH, job_service))
        {
            qFatal("Could not register Upstart object. %s", connection.lastError().message().toStdString().c_str());
        }
    }
    else
    {
        qDebug() << "Service is already registered!.";
    }
    return app.exec();
}
