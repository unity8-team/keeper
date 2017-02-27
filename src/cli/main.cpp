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
#include "command-line.h"
#include "command-line-client.h"

#include <dbus-types.h>
#include <util/logging.h>
#include "util/unix-signal-handler.h"

#include <keeper_user_interface.h>

#include <QCoreApplication>
#include <QDBusConnection>

#include <libintl.h>
#include <cstdlib>
#include <ctime>
#include <iostream>


int
main(int argc, char **argv)
{
    qInstallMessageHandler(util::loggingFunction);

    QCoreApplication app(argc, argv);
    DBusTypes::registerMetaTypes();
    std::srand(unsigned(std::time(nullptr)));

    util::UnixSignalHandler handler([]{
        CommandLineClient client;
        client.run_cancel();
    });
    handler.setupUnixSignalHandlers();

    // boilerplate locale
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
    textdomain(GETTEXT_PACKAGE);

    QCoreApplication::setApplicationName("keeper");
    QCoreApplication::setApplicationVersion("1.0");

    CommandLineParser parser;
    CommandLineClient client;
    CommandLineParser::CommandArgs cmd_args;
    if (parser.parse(QCoreApplication::arguments(), app, cmd_args))
    {
        switch(cmd_args.cmd)
        {
            case CommandLineParser::Command::LIST_LOCAL_SECTIONS:
                client.run_list_sections(false);
                exit(0);
                break;
            case CommandLineParser::Command::LIST_STORAGE_ACCOUNTS:
                client.run_list_storage_accounts();
                exit(0);
                break;
            case CommandLineParser::Command::LIST_REMOTE_SECTIONS:
                client.run_list_sections(true, cmd_args.storage);
                exit(0);
                break;
            case CommandLineParser::Command::BACKUP:
                client.run_backup(cmd_args.sections, cmd_args.storage);
                break;
            case CommandLineParser::Command::RESTORE:
                client.run_restore(cmd_args.sections, cmd_args.storage);
                break;
        };
    }


#if 0
    Factory factory;
    auto menu = factory.newMenuBuilder();
    auto connectivityService = factory.newConnectivityService();
    auto vpnStatusNotifier = factory.newVpnStatusNotifier();
#endif

    return app.exec();
}
