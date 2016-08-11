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

#include "tar/tar-creator.h"
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

QStringList
get_filenames_from_file(FILE * fp)
{
    // don't wait forever...
    int fd = fileno(fp);
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    struct timeval tv {};
    tv.tv_sec = 2;
    select(1, &readfds, NULL, NULL, &tv);
    if (!FD_ISSET(fd, &readfds)) {
        qWarning() << "Couldn't read files from stdin";
        return QStringList();
    }

    // read the file list
    QFile file;
    file.open(fp, QIODevice::ReadOnly);
    auto filenames_raw = file.readAll();
    file.close();

    // massage into a QStringList
    QStringList filenames;
    for (const auto& token : filenames_raw.split('\0'))
        if (!token.isEmpty())
            filenames.append(QString::fromUtf8(token));

    return filenames;
}

std::tuple<bool,QString,QStringList>
parse_args(QCoreApplication& app)
{
    // parse the command line
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.setApplicationDescription(
        "\n"
        "Reads filenames from the standard input, delimited either by a null character,\n"
        "builds an in-memory archive of those files, and sends them to the Keeper service\n"
        " to store remotely.\n"
        "\n"
        "You will need to insure that the program which produces input uses a null character\n"
        "as a separator. If that program is GNU find, for example, the -print0 option does\n"
        "this for you.\n"
        "\n"
        "Helper usage: find /your/data/path -print0 | "  APP_NAME " -a /bus/path"
    );
    QCommandLineOption compress_option{
        QStringList() << "c" << "compress",
        QStringLiteral("Compress files before adding to archive")
    };
    parser.addOption(compress_option);
    QCommandLineOption bus_path_option{
        QStringList() << "a" << "bus-path",
        QStringLiteral("Keeper service's DBus path"),
        QStringLiteral("bus-path")
    };
    parser.addOption(bus_path_option);
    parser.process(app);
    const bool compress = parser.isSet(compress_option);
    const auto bus_path = parser.value(bus_path_option);

    // gotta have the bus path
    if (bus_path.isEmpty()) {
        std::cerr << "Missing required argument: --bus-path" << std::endl;
        parser.showHelp(EXIT_FAILURE);
    }

    // gotta have files
    const auto filenames = get_filenames_from_file(stdin);
    for (const auto& filename : filenames)
        qDebug() << "filename:" << filename;

    return std::make_tuple(compress, bus_path, filenames);
}

QDBusUnixFileDescriptor
get_socket_from_keeper(size_t n_bytes, const QString& bus_path)
{
    QDBusUnixFileDescriptor ret;

    qDebug() << "asking keeper for a socket";
    DBusInterfaceKeeperHelper helperInterface(
        DBusTypes::KEEPER_SERVICE,
        bus_path,
        QDBusConnection::sessionBus()
    );
    auto fd_reply = helperInterface.StartBackup(n_bytes);
    fd_reply.waitForFinished();
    if (fd_reply.isError()) {
        qCritical("Call to '%s.StartBackup() at '%s' call failed: %s",
            DBusTypes::KEEPER_SERVICE,
            qPrintable(bus_path),
            qPrintable(fd_reply.error().message())
        );
    } else {
        ret = fd_reply.value();
    }

    return ret;
}

ssize_t
send_tar_to_keeper(TarCreator& tar_creator, int fd)
{
    ssize_t n_sent {};

    // send the tar to the socket piece by piece
    std::vector<char> buf;
    while(tar_creator.step(buf)) {
        const char* walk {buf.data()};
        auto n_left = size_t{buf.size()};
        while(n_left > 0) {
            const auto n_written_in = write(fd, walk, n_left);
            if (n_written_in > 0) {
                const auto n_written = size_t(n_written_in);
                walk += n_written;
                n_sent += n_written;
                n_left -= n_written;
            } else if (errno == EAGAIN) {
                QThread::msleep(100);
            } else {
                qCritical("error sending binary blob to Keeper: %s", strerror(errno));
                return -1;
            }
        }
    }

    return n_sent;
}

} // anonymous namespace

int
main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    // get the inputs
    bool compress;
    QString bus_path;
    QStringList filenames;
    std::tie(compress, bus_path, filenames) = parse_args(app);

    // build the creator
    TarCreator tar_creator{filenames, compress};
    const auto n_bytes_in = tar_creator.calculate_size();
    if (n_bytes_in < 0) {
        qCritical("Unable to estimate tar size");
        return EXIT_FAILURE;
    }
    const auto n_bytes = size_t(n_bytes_in);
    qDebug() << "tar size should be" << n_bytes;

    // do it!
    const auto qfd = get_socket_from_keeper(n_bytes, bus_path);
    if (!qfd.isValid()) {
        qCritical() << "Can't proceed without a socket from keeper";
        return EXIT_FAILURE;
    }
    const auto fd = qfd.fileDescriptor();
    const auto n_sent = send_tar_to_keeper(tar_creator, fd);
    qDebug() << "tar size was" << n_sent;

    return EXIT_SUCCESS;
}
