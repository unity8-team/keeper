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

#include "tar/untar.h"
#include "qdbus-stubs/dbus-types.h"
#include "qdbus-stubs/keeper_helper_interface.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusUnixFileDescriptor>
#include <QFile>
#include <QLocalSocket>

#include <sys/select.h>
#include <unistd.h>

#include <cstdio> // fileno()
#include <ctime>
#include <iostream>
#include <type_traits>

namespace
{

std::tuple<QString>
parse_args(QCoreApplication& app)
{
    // parse the command line
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.setApplicationDescription(
        "\n"
        "The reverse of keeper-tar. Queries Keeper for a socket fd, then pipes\n"
        "that socket through xzcat and tar to restore the archive data into the current\n"
        "working directory.\n"
        "\n"
        "Helper usage: "  APP_NAME " -a /bus/path"
    );
    QCommandLineOption bus_path_option{
        QStringList() << "a" << "bus-path",
        QStringLiteral("Keeper service's DBus path"),
        QStringLiteral("bus-path")
    };
    parser.addOption(bus_path_option);
    parser.process(app);
    const auto bus_path = parser.value(bus_path_option);

    // gotta have the bus path
    if (bus_path.isEmpty()) {
        std::cerr << "Missing required argument: --bus-path" << std::endl;
        parser.showHelp(EXIT_FAILURE);
    }

    return std::make_tuple(bus_path);
}

QDBusUnixFileDescriptor
get_socket_from_keeper(const QString& bus_path)
{
    QDBusUnixFileDescriptor ret;

    qDebug() << "asking keeper for a socket";
    DBusInterfaceKeeperHelper helperInterface(
        DBusTypes::KEEPER_SERVICE,
        bus_path,
        QDBusConnection::sessionBus()
    );

    auto fd_reply = helperInterface.StartRestore();
    fd_reply.waitForFinished();
    if (fd_reply.isError()) {
        qCritical("Call to '%s.StartRestore() at '%s' call failed: %s",
            DBusTypes::KEEPER_SERVICE,
            qPrintable(bus_path),
            qPrintable(fd_reply.error().message())
        );
    } else {
        ret = fd_reply.value();
    }

    return ret;
}

bool
untar_from_socket(Untar& untar, int fd)
{
    bool success = false;
    static constexpr int STEP_BUFSIZE = 4096*4; // arbitrary
    char buf[STEP_BUFSIZE];

    for (;;)
    {
        auto const n_read = read(fd, buf, sizeof(buf));

        if (n_read > 0)
        {
            if (!untar.step(buf, n_read))
                break;
        }
        else if (n_read == 0) // eof
        {
            qDebug() << Q_FUNC_INFO << "eof reached";
            success = true;
            break;
        }
        else if (errno == EAGAIN)
        {
            QThread::msleep(100);
            continue;
        }
        else
        {
            qCritical() << Q_FUNC_INFO << "read() returned" << strerror(errno);
            break;
        }
    }

    return success;
}

} // anonymous namespace

int
main(int argc, char **argv)
{
qInfo() << Q_FUNC_INFO << "HELLO WORLD";
    QCoreApplication app(argc, argv);

    // get the inputs
    QString bus_path;
    std::tie(bus_path) = parse_args(app);

    // ask keeper for a socket to read
    const auto qfd = get_socket_from_keeper(bus_path);
    if (!qfd.isValid()) {
        qCritical() << "Can't proceed without a socket from keeper";
        return EXIT_FAILURE;
    }

    // do it!
    auto const cwd = QDir::currentPath().toStdString();
    Untar untar{cwd};
    return untar_from_socket(untar, qfd.fileDescriptor())
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}
