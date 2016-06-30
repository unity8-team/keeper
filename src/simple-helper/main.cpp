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
 * Authors: Charles Kerr <charles.kerr@canonical.com>
 * 			Xavi Garcia <xavi.garcia.mena@canonical.com>
 */
#include <dbus-types.h>

#include "keeper_interface.h"
#include "simple-helper-defs.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusUnixFileDescriptor>
#include <QLocalSocket>

#include <libintl.h>
#include <cstdlib>
#include <ctime>

int
main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    std::srand(unsigned(std::time(nullptr)));

    // boilerplate locale
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
    textdomain(GETTEXT_PACKAGE);

    QScopedPointer<DBusInterfaceKeeper> keeperInterface(new DBusInterfaceKeeper(DBusTypes::KEEPER_SERVICE,
                                                            DBusTypes::KEEPER_SERVICE_PATH,
                                                            QDBusConnection::sessionBus(), 0));

    QDBusReply<QDBusUnixFileDescriptor> userResp = keeperInterface->call(QLatin1String("GetBackupSocketDescriptor"));

    if (!userResp.isValid())
    {
        qWarning() << "Error getting backup socket: " << userResp.error().message();
    }
    else
    {
        auto backupSocket = userResp.value().fileDescriptor();
        qDebug() << "I've got the following socket descriptor: " << backupSocket;

        QLocalSocket localSocket;
        localSocket.setSocketDescriptor(backupSocket);

        qDebug() << "Wrote " << localSocket.write(SIMPLE_HELPER_TEXT_TO_WRITE) << " bytes to it.";
        localSocket.flush();
        localSocket.disconnectFromServer();


        QFile markFile(SIMPLE_HELPER_MARK_FILE_PATH);
        markFile.open(QFile::WriteOnly);
        markFile.close();
        // this is just used when we need to simulate that the helper has finished
        // when not running the full setup with upstart
//        qDebug() << "Finishing...";
//        QDBusReply<void> finishResp = keeperInterface->call(QLatin1String("finish"));
//
//        if (!finishResp.isValid())
//        {
//            qWarning() << "Error finishing: " << finishResp.error().message();
//        }
//        else
//        {
//            qDebug() << "Finished.";
//        }
    }

    return 0;
}
