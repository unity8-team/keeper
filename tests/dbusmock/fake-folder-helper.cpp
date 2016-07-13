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

#include <qdbus-stubs/dbus-types.h>
#include <qdbus-stubs/keeper_helper_interface.h>

#include <QCoreApplication>
#include <QDebug>
#include <QProcessEnvironment>

#include <unistd.h>

#include <iostream>

int
main(int argc, char **argv)
{
    std::cout << "hello world!" << std::endl;

    QCoreApplication app(argc, argv);

    // dump the inputs to stdout
    std::cout << " argc: " << argc << std::endl;
    for(int i=0; i<argc; ++i)
        std::cout << " argv[" << i << "]: " << argv[i] << std::endl;
    const auto env = QProcessEnvironment::systemEnvironment();
    for(const auto& key : env.keys())
       std::cout << " env " << qPrintable(key) << "=\"" << qPrintable(env.value(key)) << '"' << std::endl;

    // ask the service for a socket
    auto connection = QDBusConnection::connectToBus(
        QDBusConnection::SessionBus,
        DBusTypes::KEEPER_SERVICE
    );
    DBusInterfaceKeeperHelper helper_iface (
        DBusTypes::KEEPER_SERVICE,
        QString::fromUtf8(argv[1]),
        connection
    );
    const auto n_bytes = uint64_t{1000};
    auto pending_reply = helper_iface.StartBackup(n_bytes);
    pending_reply.waitForFinished();
    Q_ASSERT(pending_reply.isValid());
    const auto unixfd = pending_reply.value();
    Q_ASSERT(unixfd.isValid());
    const auto fd = unixfd.fileDescriptor();
    const auto gronk = std::vector<char>(n_bytes, 'x');
    const char* walk = &gronk.front();
    auto n_left = n_bytes;
    do {
        const auto n_written = write(fd, walk, size_t(n_left));
        if (n_written > 0) {
            walk += n_written;
            n_left -= n_written;
        }
        else if (n_written < 0) {
            abort();
        }
    } while(n_left > 0);
}
