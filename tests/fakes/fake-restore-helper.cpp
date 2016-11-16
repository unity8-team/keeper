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

#include "fake-restore-helper.h"
#include "restore-reader.h"

#include <qdbus-stubs/dbus-types.h>
#include <qdbus-stubs/keeper_helper_interface.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QProcessEnvironment>
#include <QLocalSocket>
#include <QtGlobal>

#include <unistd.h>
#include <sys/ioctl.h>

void myMessageHandler(QtMsgType type, const QMessageLogContext &, const QString & msg)
{
    QString txt;
    switch (type) {
    case QtDebugMsg:
        txt = QString("Debug: %1").arg(msg);
        break;
    case QtWarningMsg:
        txt = QString("Warning: %1").arg(msg);
    break;
    case QtCriticalMsg:
        txt = QString("Critical: %1").arg(msg);
    break;
    case QtFatalMsg:
        txt = QString("Fatal: %1").arg(msg);
        abort();
    }
    QFile outFile("/tmp/restore-helper-output");
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << endl;
}

int
main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    qInstallMessageHandler(myMessageHandler);

    // dump the inputs to stdout
    qDebug() << "argc:" << argc;
    for(int i=0; i<argc; ++i)
        qDebug() << "argv[" << i << "] is" << argv[i];
    const auto env = QProcessEnvironment::systemEnvironment();
    for(const auto& key : env.keys())
        qDebug() << "env" << qPrintable(key) << "is" << qPrintable(env.value(key));

    qDebug() << "Retrieving connection";

    // ask the service for a socket
    auto conn = QDBusConnection::connectToBus(QDBusConnection::SessionBus, DBusTypes::KEEPER_SERVICE);
    const auto object_path = QString::fromUtf8(DBusTypes::KEEPER_HELPER_PATH);
    DBusInterfaceKeeperHelper helper_iface (DBusTypes::KEEPER_SERVICE, object_path, conn);

    qDebug() << "Is valid:" << helper_iface.isValid();

    auto fd_reply = helper_iface.StartRestore();
    fd_reply.waitForFinished();
    if (fd_reply.isError())
    {
        qFatal("Call to '%s.StartRestore() at '%s' call failed: %s",
            DBusTypes::KEEPER_SERVICE,
            qPrintable(object_path),
            qPrintable(fd_reply.error().message())
        );
    }
    const auto ufd = fd_reply.value();

    // write the blob
    const auto fd = ufd.fileDescriptor();
    qDebug() << "The file descriptor obtained is: " << fd;
    RestoreReader reader(fd, TEST_RESTORE_FILE_PATH);
    return app.exec();
}
