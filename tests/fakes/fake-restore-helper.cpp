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

#include "fake-restore-helper.h"

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

constexpr int UPLOAD_BUFFER_MAX_ = 64 * 1024;

namespace
{
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
    case QtInfoMsg:
           txt = QString("Info: %1").arg(msg);
           break;
    case QtFatalMsg:
        txt = QString("Fatal: %1").arg(msg);
        abort();
    }
    QFile outFile(TEST_RESTORE_LOG_FILE_PATH);
    outFile.open(QIODevice::WriteOnly);
    QTextStream ts(&outFile);
    ts << txt << endl;
}
} // namespace

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
    QSharedPointer<DBusInterfaceKeeperHelper> helper_iface (new DBusInterfaceKeeperHelper(DBusTypes::KEEPER_SERVICE, object_path, conn));

    qDebug() << "Is valid:" << helper_iface->isValid();

    auto fd_reply = helper_iface->StartRestore();
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
    const auto fd = ufd.fileDescriptor();
    qDebug() << "The file descriptor obtained is: " << fd;

    char buffer[UPLOAD_BUFFER_MAX_];
    size_t n_bytes_read = 0;
    QFile file(TEST_RESTORE_FILE_PATH);
    file.open(QIODevice::WriteOnly);
    for(;;)
    {
        // Read data into buffer.  We may not have enough to fill up buffer, so we
        // store how many bytes were actually read in bytes_read.
        auto bytes_read = read(fd, buffer, sizeof(buffer));
        if (bytes_read == 0) // We're done reading from the file
        {
            qDebug() << "Returned 0 bytes read";
            QCoreApplication::exit(0);
            break;
        }

        else if (bytes_read < 0)
        {
            if (errno != EAGAIN)
                qWarning() << "Error reading from the socket: " << strerror(errno);
        }
        else
        {
            n_bytes_read += bytes_read;
            qDebug() << "Read: " << bytes_read << " Total: " << n_bytes_read;
            file.write(buffer, bytes_read);
            file.flush();
        }
    }
    file.close();
    return 0;
}
