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
    constexpr const char ARGUMENT_LIST_SECTIONS[]         = "list-sections";
    constexpr const char ARGUMENT_LIST_STORAGE_ACCOUNTS[] = "list-storage-configs";
    constexpr const char ARGUMENT_BACKUP[]                = "backup";
    constexpr const char ARGUMENT_RESTORE[]               = "restore";

    // argument descriptions
    constexpr const char ARGUMENT_LIST_SECTIONS_DESCRIPTION[]         = "List the sections available to backup";
    constexpr const char ARGUMENT_LIST_STORAGE_ACCOUNTS_DESCRIPTION[] = "List the available storage accounts";
    constexpr const char ARGUMENT_BACKUP_DESCRIPTION[]                = "Starts a backup";
    constexpr const char ARGUMENT_RESTORE_DESCRIPTION[]               = "Starts a restore";

    // options
    constexpr const char OPTION_STORAGE[]          = "storage";
    constexpr const char OPTION_SECTIONS[]         = "sections";

    // option descriptions
    constexpr const char OPTION_STORAGE_DESCRIPTION[]          = "Defines the available storage to use. Pass 'default' to use the default one";
    constexpr const char OPTION_SECTIONS_DESCRIPTION[]         = "Lists the sections to backup or restore";
}

CommandLineParser::CommandLineParser()
    : parser_(new QCommandLineParser)
{
    parser_->setApplicationDescription("Keeper command line client");
    parser_->addHelpOption();
    parser_->addVersionOption();
    parser_->addPositionalArgument(ARGUMENT_LIST_SECTIONS, QCoreApplication::translate("main", ARGUMENT_LIST_SECTIONS_DESCRIPTION));
    parser_->addPositionalArgument(ARGUMENT_LIST_STORAGE_ACCOUNTS, QCoreApplication::translate("main", ARGUMENT_LIST_STORAGE_ACCOUNTS_DESCRIPTION));
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
        else if (args.at(0) == ARGUMENT_LIST_STORAGE_ACCOUNTS)
        {
            return handle_list_storage_accounts(app, cmd_args);
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
                QCoreApplication::translate("main", OPTION_STORAGE_DESCRIPTION),
                QCoreApplication::translate("main", OPTION_STORAGE_DESCRIPTION)
            },
        });
    parser_->process(app);

    // it didn't exit... we're good
    cmd_args.sections.clear();
    cmd_args.storage.clear();
    if (parser_->isSet(OPTION_STORAGE))
    {
        cmd_args.cmd = CommandLineParser::Command::LIST_REMOTE_SECTIONS;
        cmd_args.storage = get_storage_string(parser_->value(OPTION_STORAGE));
    }
    else
    {
        cmd_args.cmd = CommandLineParser::Command::LIST_LOCAL_SECTIONS;
    }

    return true;
}

bool CommandLineParser::handle_list_storage_accounts(QCoreApplication const & app, CommandArgs & cmd_args)
{
    parser_->process(app);
    // it didn't exit... we're good
    cmd_args.sections.clear();
    cmd_args.storage.clear();
    cmd_args.cmd = CommandLineParser::Command::LIST_STORAGE_ACCOUNTS;

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
            {{"r", OPTION_STORAGE},
                QCoreApplication::translate("main", OPTION_STORAGE_DESCRIPTION),
                QCoreApplication::translate("main", OPTION_STORAGE_DESCRIPTION)
            },
        });
    parser_->process(app);

    // it didn't exit... we're good
    cmd_args.sections.clear();
    cmd_args.storage.clear();
    cmd_args.cmd = CommandLineParser::Command::BACKUP;
    if (!parser_->isSet(OPTION_SECTIONS))
    {
        std::cerr << "You need to specify some sections to run a backup." << std::endl;
        return false;
    }
    if (parser_->isSet(OPTION_STORAGE))
    {
        cmd_args.storage = get_storage_string(parser_->value(OPTION_STORAGE));
    }
    cmd_args.sections = parser_->value(OPTION_SECTIONS).split(',');

    return true;
}

bool CommandLineParser::handle_restore(QCoreApplication const & app, CommandLineParser::CommandArgs & cmd_args)
{
    parser_->clearPositionalArguments();
    parser_->addPositionalArgument(ARGUMENT_RESTORE, QCoreApplication::translate("main", ARGUMENT_RESTORE_DESCRIPTION));

    parser_->addOptions({
            {{"s", OPTION_SECTIONS},
                QCoreApplication::translate("main", OPTION_SECTIONS_DESCRIPTION),
                QCoreApplication::translate("main", OPTION_SECTIONS_DESCRIPTION)
            },
            {{"r", OPTION_STORAGE},
                QCoreApplication::translate("main", OPTION_STORAGE_DESCRIPTION),
                QCoreApplication::translate("main", OPTION_STORAGE_DESCRIPTION)
            },
        });
    parser_->process(app);

    // it didn't exit... we're good
    cmd_args.sections.clear();
    cmd_args.storage.clear();
    cmd_args.cmd = CommandLineParser::Command::RESTORE;
    if (!parser_->isSet(OPTION_SECTIONS))
    {
        std::cerr << "You need to specify some sections to run a restore." << std::endl;
        return false;
    }
    if (parser_->isSet(OPTION_STORAGE))
    {
        cmd_args.storage = get_storage_string(parser_->value(OPTION_STORAGE));
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

QString CommandLineParser::get_storage_string(QString const & value)
{
    if (value == "default")
    {
        return QString();
    }
    return value;
}
