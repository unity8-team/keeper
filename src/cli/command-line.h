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
#pragma once

#include <QCoreApplication>
#include <QSharedPointer>

class QCommandLineParser;

class CommandLineParser
{
public:
    Q_ENUMS(Command)
    enum class Command {LIST_LOCAL_SECTIONS, LIST_REMOTE_SECTIONS, BACKUP, RESTORE};
    struct CommandArgs
    {
        Command cmd;
        QStringList sections;
        QString storage;
    };

    CommandLineParser();
    ~CommandLineParser() = default;
    Q_DISABLE_COPY(CommandLineParser)

    bool parse(QStringList const & arguments, QCoreApplication const & app, CommandArgs & cmd_args);

private:
    bool handle_list_sections(QCoreApplication const & app, CommandArgs & cmd_args);
    bool handle_backup(QCoreApplication const & app, CommandArgs & cmd_args);
    bool handle_restore(QCoreApplication const & app, CommandArgs & cmd_args);

    bool check_number_of_args(QStringList const & args);

    QSharedPointer<QCommandLineParser> parser_;
};
