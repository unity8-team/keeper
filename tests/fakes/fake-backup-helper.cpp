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
 */

#include "fake-backup-helper.h"

#include <qdbus-stubs/dbus-types.h>
#include <qdbus-stubs/keeper_helper_interface.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QProcessEnvironment>
#include <QLocalSocket>

int
main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const auto blob = QByteArray(FAKE_BACKUP_HELPER_PAYLOAD);

    // dump the inputs to stdout
    qDebug() << "argc:" << argc;
    for(int i=0; i<argc; ++i)
        qDebug() << "argv[" << i << "] is" << argv[i];
    const auto env = QProcessEnvironment::systemEnvironment();
    for(const auto& key : env.keys())
        qDebug() << "env" << qPrintable(key) << "is" << qPrintable(env.value(key));

    // ask the service for a socket
    auto conn = QDBusConnection::connectToBus(QDBusConnection::SessionBus, DBusTypes::KEEPER_SERVICE);
    const auto object_path = QString::fromUtf8(argv[1]);
    DBusInterfaceKeeperHelper helper_iface (DBusTypes::KEEPER_SERVICE, object_path, conn);
    QDBusReply<QDBusUnixFileDescriptor> reply = helper_iface.call("StartBackup", blob.size());
    const auto ufd = reply.value();
    Q_ASSERT(reply.isValid());

    // write the blob
    QLocalSocket sock;
    sock.setSocketDescriptor(ufd.fileDescriptor());
    qDebug() << "wrote" << sock.write(blob) << "bytes";
    sock.flush();

    // create the mark file so we can check when it finished without upstart
    QFile markFile(SIMPLE_HELPER_MARK_FILE_PATH);
    markFile.open(QFile::WriteOnly);
    markFile.close();

    return EXIT_SUCCESS;
}
