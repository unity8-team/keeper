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
 *     Charles Kerr <charles.kerr@canonical.com>
 *     Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#include "dbus-types.h"
#include "service/backup-choices.h"
#include "service/restore-choices.h"
#include "service/keeper.h"
#include "service/keeper-user.h"
#include "util/logging.h"
#include "util/unix-signal-handler.h"
#include "helper/helper.h"

#include "KeeperAdaptor.h"
#include "KeeperUserAdaptor.h"
#include "KeeperHelperAdaptor.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

#include <libintl.h>
#include <cstdlib>
#include <ctime>

int
main(int argc, char **argv)
{
    qInstallMessageHandler(util::loggingFunction);

    QCoreApplication app(argc, argv);
    DBusTypes::registerMetaTypes();
//    Helper::registerMetaTypes();
    std::srand(unsigned(std::time(nullptr)));

    util::UnixSignalHandler handler([]{
        QCoreApplication::exit(0);
    });
    handler.setupUnixSignalHandlers();

    // boilerplate locale
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
    textdomain(GETTEXT_PACKAGE);

    if (argc == 2 && QString("--print-address") == argv[1])
    {
        qDebug() << QDBusConnection::sessionBus().baseService();
    }

    // dbus service setup
    QDBusConnection connection = QDBusConnection::sessionBus();

    if (connection.interface()->isServiceRegistered(DBusTypes::KEEPER_SERVICE))
    {
        qDebug() << "Service is already registered";
    }
    else
    {
        // register the service
        if (!connection.registerService(DBusTypes::KEEPER_SERVICE))
        {
            qFatal("Could not register keeper dbus service: [%s]", connection.lastError().message().toStdString().c_str());
            return 1;
        }

        QSharedPointer<MetadataProvider> possible (new BackupChoices());
        QSharedPointer<MetadataProvider> available (new RestoreChoices());
        auto service = new Keeper(possible, available, &app);
        new KeeperAdaptor(service);
        if (!connection.registerObject(DBusTypes::KEEPER_SERVICE_PATH, service))
        {
            qFatal("Could not register keeper dbus service object: [%s]", connection.lastError().message().toStdString().c_str());
            return 1;
        }

        // register the helper object
        auto helper  = new KeeperHelper(service);
        new KeeperHelperAdaptor(helper);
        if (!connection.registerObject(DBusTypes::KEEPER_HELPER_PATH, helper))
        {
            qFatal("Could not register keeper dbus helper object: [%s]", connection.lastError().message().toStdString().c_str());
            return 1;
        }

        // register the user object
        auto user  = new KeeperUser(service);
        new KeeperUserAdaptor(user);
        if (!connection.registerObject(DBusTypes::KEEPER_USER_PATH, user))
        {
            qFatal("Could not register keeper dbus user object: [%s]", connection.lastError().message().toStdString().c_str());
            return 1;
        }
    }


    return app.exec();
}
