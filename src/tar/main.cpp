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

//#include "util/logging.h"
//#include "util/unix-signal-handler.h"

#include "tar/tar-builder.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

#include <ctime>
#include <iostream>

int
main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    //QCoreApplication::setApplicationName("keeper--copy-program");
    //QCoreApplication::setApplicationVersion("1.0");
 //   qInstallMessageHandler(util::loggingFunction);

    QCommandLineParser parser;
    parser.setApplicationDescription("Backup files helper for Keeper");
    parser.addHelpOption();

    QCommandLineOption compressOption(
        QStringList() << "c" << "compress",
        QStringLiteral("Compress files before adding to archive")
    );
    parser.addOption(compressOption);

    QCommandLineOption pathOption(
        QStringList() << "a" << "bus-path",
        QStringLiteral("Keeper service's DBus path"),
        QStringLiteral("bus-path")
    );
    parser.addOption(pathOption);
    parser.addPositionalArgument("files", "The files/directories to back up.");

    parser.process(app);

    const auto files = parser.positionalArguments();
    if (files.isEmpty()) {
        parser.showHelp(EXIT_FAILURE);
    }

    const bool compress = parser.isSet(compressOption);
    std::cerr << "compress: " << compress << std::endl;

    for (const auto& file : files) {
        std::cerr << "--> " << qPrintable(file) << std::endl;
    }

    TarBuilder tar_builder(files, compress);
    std::cout << "size: " << tar_builder.calculate_size() << std::endl;


    //DBusTypes::registerMetaTypes();
// FIXME: why?
    std::srand(unsigned(std::time(nullptr)));

#if 0
    // boilerplate locale
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
    textdomain(GETTEXT_PACKAGE);
#endif


    return EXIT_SUCCESS;//app.exec();
}
