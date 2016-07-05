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
#include "qdbus-stubs/keeper_interface.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusUnixFileDescriptor>
#include <QFile>
#include <QLocalSocket>

#include <glib.h>

#include <ctime>
#include <iostream>

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
        auto filenames_raw_zeroterminated = QString(filenames_raw).toUtf8();
        g_shell_parse_argv(filenames_raw_zeroterminated.constData(), nullptr, &filenames_strv, &err);
        if (err != nullptr)
            g_warning("Unable to parse file list: %s", err->message);
        for(int i=0; filenames_strv && filenames_strv[i]; ++i)
            tokens.append(QByteArray(filenames_strv[i]));
        g_clear_pointer(&filenames_strv, g_strfreev);
        g_clear_error(&err);
    }

    QStringList filenames;
    for (const auto& token : tokens)
        filenames.append(QString::fromUtf8(token));
  
    return filenames;
}

} // anonymous namespace

int
main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    // parse the command line
    // FIXME: do we want i18n?
    QCommandLineParser parser;
    parser.setApplicationDescription("Backup files helper for Keeper");
    parser.addHelpOption();
    QCommandLineOption compress_option(
        QStringList() << "c" << "compress",
        QStringLiteral("Compress files before adding to archive")
    );
    QCommandLineOption zero_delimiter_option(
        QStringList() << "0" << "null",
        QStringLiteral("Input items are terminated by a null character instead of by whitespace")
    );
    parser.addOption(zero_delimiter_option);
    QCommandLineOption path_option(
        QStringList() << "a" << "bus-path",
        QStringLiteral("Keeper service's DBus path"),
        QStringLiteral("bus-path")
    );
    parser.addOption(path_option);
#if 0
    parser.addPositionalArgument("files", "The files/directories to back up.");
#endif
    parser.process(app);
    const bool compress = parser.isSet(compress_option);
    const bool zero = parser.isSet(zero_delimiter_option);
    const auto bus_path = parser.value(bus_path_option);

    // gotta have the bus path
    if (bus_path.empty())
    {
        std::cerr << "bus-path not listed" << std::endl;
        return EXIT_FAILURE;
    }
    
    // gotta have files
    const auto filenames = get_filenames_from_file(stdin, zero);
    for (const auto& filename : filenames)
    {
        qDebug() << "filename: " << filename;
    }
    if (filenames.empty())
    {
        std::cerr << "no files listed" << std::endl;
        return EXIT_FAILURE;
    }

    // build the creator
    TarCreator tar_creator(filenames, compress);
    qDebug() << "tar size will be: " << tar_creator.calculate_size();

    QScopedPointer<DBusInterfaceKeeper> keeperInterface(
        new DBusInterfaceKeeper(
            DBusTypes::KEEPER_SERVICE,
            DBusTypes::KEEPER_SERVICE_PATH,
            QDBusConnection::sessionBus(),
            0
        )
    );

    QDBusReply<QDBusUnixFileDescriptor> userResp = keeperInterface->call(QLatin1String("StartBackup"));
    if (!userResp.isValid())
    {
        qWarning() << "Error getting backup socket: " << userResp.error().message();
    }
    else
    {
        auto backupSocket = userResp.value().fileDescriptor();
        qDebug() << "I've got the following socket descriptor: " << backupSocket;

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out << "This is a test\n";
        QLocalSocket localSocket;
        localSocket.setSocketDescriptor(backupSocket);

        qDebug() << "Wrote " << localSocket.write(block) << " bytes to it.";
        localSocket.flush();
        localSocket.disconnectFromServer();




    //
#if 0
    const auto files = parser.positionalArguments();
    if (files.isEmpty()) {
        parser.showHelp(EXIT_FAILURE);
    }
#endif

#if 0
#endif


#include <libintl.h>
#include <cstdlib>
#include <ctime>

int
main(int argc, char **argv)
{
zzz
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

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out << "This is a test\n";
        QLocalSocket localSocket;
        localSocket.setSocketDescriptor(backupSocket);

        qDebug() << "Wrote " << localSocket.write(block) << " bytes to it.";
        localSocket.flush();
        localSocket.disconnectFromServer();


    return EXIT_SUCCESS;
}
