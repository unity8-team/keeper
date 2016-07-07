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

#include <glib.h>

#include <unistd.h>

#include <ctime>
#include <iostream>
#include <type_traits>

namespace
{

QStringList
get_filenames_from_file(FILE * fp, bool zero)
{
    QFile file;
    file.open(fp, QIODevice::ReadOnly);
    auto filenames_raw = file.readAll();
    file.close();

    QList<QByteArray> tokens;
    if (zero)
    {
        tokens = filenames_raw.split('\0');
    }
    else
    {
        // can't find a Qt equivalent of g_shell_parse_argv()...
        gchar** filenames_strv {};
        GError* err {};
        filenames_raw = QString(filenames_raw).toUtf8(); // ensure a trailing '\0'
        g_shell_parse_argv(filenames_raw.constData(), nullptr, &filenames_strv, &err);
        if (err != nullptr)
            g_warning("Unable to parse file list: %s", err->message);
        for(int i=0; filenames_strv && filenames_strv[i]; ++i)
            tokens.append(QByteArray(filenames_strv[i]));
        g_clear_pointer(&filenames_strv, g_strfreev);
        g_clear_error(&err);
    }

    // massage into a QStringList
    QStringList filenames;
    for (const auto& token : tokens)
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
        "Reads filenames from the standard input, delimited either by blanks (which can\n"
        "be protected with double or single quotes or a backslash), builds an in-memory\n"
        "archive of those files, and sends them to the Keeper service to store remotely.\n"
        "\n"
        "Because Unix filenames can contain blanks and newlines, it is generally better\n"
        "to use the -0 option, which prevents such problems. When using this option you\n"
        "will need to ensure the program which produces input also uses a null character\n"
        "a separator. If that program is GNU find, for example, the -print0 option does\n"
        "this for you.\n"
        "\n"
        "Helper usage: find /your/data/path -print0 | "  APP_NAME " -0 -a /bus/path"
    );
    QCommandLineOption compress_option{
        QStringList() << "c" << "compress",
        QStringLiteral("Compress files before adding to archive")
    };
    parser.addOption(compress_option);
    QCommandLineOption zero_delimiter_option{
        QStringList() << "0" << "null",
        QStringLiteral("Input items are terminated by a null character instead of by whitespace")
    };
    parser.addOption(zero_delimiter_option);
    QCommandLineOption bus_path_option{
        QStringList() << "a" << "bus-path",
        QStringLiteral("Keeper service's DBus path"),
        QStringLiteral("bus-path")
    };
    parser.addOption(bus_path_option);
    parser.process(app);
    const bool compress = parser.isSet(compress_option);
    const bool zero = parser.isSet(zero_delimiter_option);
    const auto bus_path = parser.value(bus_path_option);

    // gotta have the bus path
    if (bus_path.isEmpty()) {
        std::cerr << "Missing required argument: --bus-path " << std::endl;
        parser.showHelp(EXIT_FAILURE);
    }

    // gotta have files
    const auto filenames = get_filenames_from_file(stdin, zero);
    for (const auto& filename : filenames)
        qDebug() << "filename: " << filename;
    if (filenames.empty()) {
        std::cerr << "no files listed" << std::endl;
        parser.showHelp(EXIT_FAILURE);
    }

    return std::make_tuple(compress, bus_path, filenames);
}

int
get_socket_from_keeper(size_t n_bytes, const QString& bus_path)
{
    qDebug() << "asking keeper for a socket";
    DBusInterfaceKeeperHelper helperInterface(
        DBusTypes::KEEPER_SERVICE,
        bus_path,
        QDBusConnection::sessionBus()
    );
    qDebug() << "asking keeper for a socket";
    auto fd_reply = helperInterface.StartBackup(n_bytes);
    fd_reply.waitForFinished();
    if (fd_reply.isError()) {
        qFatal("Call to '%s.StartBackup() at '%s' call failed: %s",
            DBusTypes::KEEPER_SERVICE,
            qPrintable(bus_path),
            qPrintable(fd_reply.error().message())
        );
    }
    QDBusUnixFileDescriptor qfd = fd_reply.value();
    const auto fd = qfd.fileDescriptor();
    qDebug() << "socket is" << fd;
    return fd;
}

size_t
send_tar_to_keeper(TarCreator& tar_creator, int fd)
{
    size_t n_sent {};

    // send the tar to the socket piece by piece
    std::vector<char> buf;
    while(tar_creator.step(buf)) {
        if (buf.empty())
            continue;
        const char* walk = &buf.front();
        auto n_left = size_t{buf.size()};
        while(n_left > 0) {
            const auto n_written_in = write(fd, walk, n_left);
            if (n_written_in < 0)
                qFatal("error sending binary blob to Keeper: %s", strerror(errno));
            const auto n_written = size_t(n_written_in);
            walk += n_written;
            n_sent += n_written;
            n_left += n_written;
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
    if (n_bytes_in < 0)
        qFatal("Unable to estimate tar size");
    const auto n_bytes = size_t(n_bytes_in);
    qDebug() << "tar size should be" << n_bytes;

    // do it!
    const auto fd = get_socket_from_keeper(n_bytes, bus_path);
    const auto n_sent = send_tar_to_keeper(tar_creator, fd);
    qDebug() << "tar size was" << n_sent;

    return EXIT_SUCCESS;
}
