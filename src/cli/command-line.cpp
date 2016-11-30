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
#include "command-line.h"

#include <QCommandLineParser>
#include <QDebug>

#include <iostream>

namespace
{
    // arguments
    constexpr const char ARGUMENT_LIST_SECTIONS[] = "list-sections";
    constexpr const char ARGUMENT_BACKUP[]        = "backup";
    constexpr const char ARGUMENT_RESTORE[]       = "restore";

    // argument descriptions
    constexpr const char ARGUMENT_LIST_SECTIONS_DESCRIPTION[] = "List the sections available to backup";
    constexpr const char ARGUMENT_BACKUP_DESCRIPTION[]        = "Starts a backup";
    constexpr const char ARGUMENT_RESTORE_DESCRIPTION[]       = "Starts a restore";

    // options
    constexpr const char OPTION_STORAGE[]          = "storage";
    constexpr const char OPTION_SECTIONS[]         = "sections";

    // option descriptions
    constexpr const char OPTION_STORAGE_DESCRIPTION[]          = "Lists the available sections stored at the storage";
    constexpr const char OPTION_SECTIONS_DESCRIPTION[]         = "Lists the sections to backup or restore";
}

CommandLineParser::CommandLineParser()
    : parser_(new QCommandLineParser)
{
    parser_->setApplicationDescription("Keeper command line client");
    parser_->addHelpOption();
    parser_->addVersionOption();
    parser_->addPositionalArgument(ARGUMENT_LIST_SECTIONS, QCoreApplication::translate("main", ARGUMENT_LIST_SECTIONS_DESCRIPTION));
    parser_->addPositionalArgument(ARGUMENT_BACKUP, QCoreApplication::translate("main", ARGUMENT_BACKUP_DESCRIPTION));
    parser_->addPositionalArgument(ARGUMENT_RESTORE, QCoreApplication::translate("main", ARGUMENT_RESTORE_DESCRIPTION));
}

bool CommandLineParser::parse(QStringList const & arguments, QCoreApplication const & app, CommandLineParser::CommandArgs & cmd_args)
{
    parser_->parse(arguments);
    const QStringList args = parser_->positionalArguments();
    if (!check_number_of_args(args))
    {
        return false;
    }

    if (args.size() == 1)
    {
        // if something fails at the process call the process exists
        if (args.at(0) == ARGUMENT_LIST_SECTIONS)
        {
            return handle_list_sections(app, cmd_args);
        }
        else if (args.at(0) == ARGUMENT_BACKUP)
        {
            return handle_backup(app, cmd_args);
        }
        else if (args.at(0) == ARGUMENT_RESTORE)
        {
            return handle_restore(app, cmd_args);
        }
        else
        {
            std::cerr << "Bad argument." << std::endl;
            std::cerr << parser_->errorText().toStdString() << std::endl;
            exit(1);
        }
    }
    else
    {
        qDebug() << "More or none arguments";
        // maybe we have the version or help options
        parser_->process(app);
    }

    return false;
}

bool CommandLineParser::handle_list_sections(QCoreApplication const & app, CommandLineParser::CommandArgs & cmd_args)
{
    parser_->clearPositionalArguments();
    parser_->addPositionalArgument(ARGUMENT_LIST_SECTIONS, QCoreApplication::translate("main", ARGUMENT_LIST_SECTIONS_DESCRIPTION));

    parser_->addOptions({
            {{"r", OPTION_STORAGE},
                QCoreApplication::translate("main", OPTION_STORAGE_DESCRIPTION)},
        });
    parser_->process(app);

    // it didn't exit... we're good
    cmd_args.sections = QStringList();
    cmd_args.storage = QString();
    if (parser_->isSet(OPTION_STORAGE))
    {
        cmd_args.cmd = CommandLineParser::Command::LIST_REMOTE_SECTIONS;
    }
    else
    {
        cmd_args.cmd = CommandLineParser::Command::LIST_LOCAL_SECTIONS;
    }

    return true;
}

bool CommandLineParser::handle_backup(QCoreApplication const & app, CommandLineParser::CommandArgs & cmd_args)
{
    parser_->clearPositionalArguments();
    parser_->addPositionalArgument(ARGUMENT_BACKUP, QCoreApplication::translate("main", ARGUMENT_BACKUP_DESCRIPTION));

    parser_->addOptions({
            {{"s", OPTION_SECTIONS},
                QCoreApplication::translate("main", OPTION_SECTIONS_DESCRIPTION),
                QCoreApplication::translate("main", OPTION_SECTIONS_DESCRIPTION)
            },
        });
    parser_->process(app);

    // it didn't exit... we're good
    cmd_args.sections = QStringList();
    cmd_args.storage = QString();
    cmd_args.cmd = CommandLineParser::Command::BACKUP;
    if (!parser_->isSet(OPTION_SECTIONS))
    {
        std::cerr << "You need to specify some sections to run a backup." << std::endl;
        return false;
    }
    cmd_args.sections = parser_->value(OPTION_SECTIONS).split(',');

    return true;
}

bool CommandLineParser::handle_restore(QCoreApplication const & app, CommandLineParser::CommandArgs & cmd_args)
{
    qDebug() << "Handling backup";
    parser_->clearPositionalArguments();
    parser_->addPositionalArgument(ARGUMENT_RESTORE, QCoreApplication::translate("main", ARGUMENT_RESTORE_DESCRIPTION));

    parser_->addOptions({
            {{"s", OPTION_SECTIONS},
                QCoreApplication::translate("main", OPTION_SECTIONS_DESCRIPTION),
                QCoreApplication::translate("main", OPTION_SECTIONS_DESCRIPTION)
            },
        });
    parser_->process(app);

    // it didn't exit... we're good
    cmd_args.sections = QStringList();
    cmd_args.storage = QString();
    cmd_args.cmd = CommandLineParser::Command::RESTORE;
    if (!parser_->isSet(OPTION_SECTIONS))
    {
        std::cerr << "You need to specify some sections to run a restore." << std::endl;
        return false;
    }
    cmd_args.sections = parser_->value(OPTION_SECTIONS).split(',');

    return true;
}

bool CommandLineParser::check_number_of_args(QStringList const & args)
{
    if (args.size() > 1)
    {
        std::cerr << "Please, pass only one argument." << std::endl;
        std::cerr << parser_->helpText().toStdString() << std::endl;
        return false;
    }
    return true;
}
