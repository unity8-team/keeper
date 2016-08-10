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

#include <dbus-types.h>
#include <util/logging.h>

#include "keeper_interface.h"
#include "keeper_user_interface.h"

#include <QCoreApplication>
#include <QDBusConnection>

#include <libintl.h>
#include <cstdlib>
#include <ctime>

int
main(int argc, char **argv)
{
    qInstallMessageHandler(util::loggingFunction);

    QCoreApplication app(argc, argv);
    DBusTypes::registerMetaTypes();
//    Variant::registerMetaTypes();
    std::srand(unsigned(std::time(nullptr)));

    // boilerplate locale
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
    textdomain(GETTEXT_PACKAGE);

    if (argc == 2 && QString("--print-address") == argv[1])
    {
        qDebug() << QDBusConnection::sessionBus().baseService();
    }

    qDebug() << "Argc =" << argc;
    if (argc == 2 && QString("--use-uuids") == argv[1])
    {
        QScopedPointer<DBusInterfaceKeeperUser> user_iface(new DBusInterfaceKeeperUser(
                                                                DBusTypes::KEEPER_SERVICE,
                                                                DBusTypes::KEEPER_USER_PATH,
                                                                QDBusConnection::sessionBus()
                                                            ) );
        QDBusReply<QVariantDictMap> choices = user_iface->call("GetBackupChoices");
        if (!choices.isValid())
        {
            qWarning() << "Error getting backup choices:" << choices.error().message();
        }

        QStringList uuids;
        auto choices_values = choices.value();
        for(auto iter = choices_values.begin(); iter != choices_values.end(); ++iter)
        {
            const auto& values = iter.value();
            auto iter_values = values.find("type");
            if (iter_values != values.end())
            {
                if (iter_values.value().toString() == "folder")
                {
                    // got it
                    qDebug() << "Adding uuid" << iter.key() << "with type:" << "folder";
                    uuids << iter.key();
                }
            }
        }

        QDBusReply<void> backup_reply = user_iface->call("StartBackup", uuids);

        if (!backup_reply.isValid())
        {
            qWarning() << "Error starting backup:" << backup_reply.error().message();
        }
    }
    else
    {
        QScopedPointer<DBusInterfaceKeeper> keeperInterface(new DBusInterfaceKeeper(DBusTypes::KEEPER_SERVICE,
                                                                DBusTypes::KEEPER_SERVICE_PATH,
                                                                QDBusConnection::sessionBus(), 0));

        QDBusReply<void> userResp = keeperInterface->call(QLatin1String("start"));

        if (!userResp.isValid())
        {
            qWarning() << "Error starting backup:" << userResp.error().message();
        }
    }


#if 0
    Factory factory;
    auto menu = factory.newMenuBuilder();
    auto connectivityService = factory.newConnectivityService();
    auto vpnStatusNotifier = factory.newVpnStatusNotifier();
#endif

    return app.exec();
}
