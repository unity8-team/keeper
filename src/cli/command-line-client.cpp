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
#include "command-line-client.h"
#include "command-line-client-view.h"

#include <client/client.h>

#include <QCoreApplication>
#include <QDebug>

#include <iostream>
#include <iomanip>

CommandLineClient::CommandLineClient(QObject * parent)
    : QObject(parent)
    , keeper_client_(new KeeperClient(this))
    , view_(new CommandLineClientView(this))
{
    connect(keeper_client_.data(), &KeeperClient::statusChanged, this, &CommandLineClient::on_status_changed);
    connect(keeper_client_.data(), &KeeperClient::progressChanged, this, &CommandLineClient::on_progress_changed);
    connect(keeper_client_.data(), &KeeperClient::finished, this, &CommandLineClient::on_keeper_client_finished);
    connect(keeper_client_.data(), &KeeperClient::taskStatusChanged, view_.data(), &CommandLineClientView::on_task_state_changed);
}

CommandLineClient::~CommandLineClient() = default;

void CommandLineClient::run_list_sections(bool remote)
{
    keeper::KeeperItemsMap choices_values;
    keeper::KeeperError error;
    if(!remote)
    {
        choices_values = keeper_client_->getBackupChoices(error);
        check_for_choices_error(error);
        list_backup_sections(choices_values);
    }
    else
    {
        choices_values = keeper_client_->getRestoreChoices(error);
        check_for_choices_error(error);
        list_restore_sections(choices_values);
    }
}

void CommandLineClient::run_backup(QStringList & sections)
{
    auto unhandled_sections = sections;
    keeper::KeeperError error;
    auto choices_values = keeper_client_->getBackupChoices(error);
    check_for_choices_error(error);
    QStringList uuids;

    auto uuids_choices = choices_values.get_uuids();
    for(auto iter = uuids_choices.begin(); iter != uuids_choices.end() && unhandled_sections.size(); ++iter)
    {
        const auto& values = choices_values[(*iter)];

        if (values.is_valid() && values.get_type() == "folder")
        {

            auto display_name = values.get_display_name();
            auto index = unhandled_sections.indexOf(display_name);
            if (index != -1)
            {
                // we have to backup this section
                uuids << (*iter);
                unhandled_sections.removeAt(index);
                view_->add_task(display_name, "waiting", 0.0);
            }
        }
    }

    if (!unhandled_sections.isEmpty())
    {
        QString error_message("The following sections were not found: \n");
        for (auto const & section : unhandled_sections)
        {
            error_message += QStringLiteral("\t %1 \n").arg(section);
        }
        view_->print_error_message(error_message);
        exit(1);
    }

    for (auto const & uuid: uuids)
    {
        keeper_client_->enableBackup(uuid, true);
    }
    keeper_client_->startBackup();
    view_->start_printing_tasks();
}

void CommandLineClient::run_restore(QStringList & sections)
{
    auto unhandled_sections = sections;
    keeper::KeeperError error;
    auto choices_values = keeper_client_->getRestoreChoices(error);
    check_for_choices_error(error);
    QStringList uuids;

    auto uuids_choices = choices_values.get_uuids();
    for(auto iter = uuids_choices.begin(); iter != uuids_choices.end(); ++iter)
    {
        const auto& values = choices_values[(*iter)];

        if (values.is_valid() && values.get_type() == "folder")
        {
            auto display_name = values.get_display_name();
            auto dir_name = values.get_dir_name();

            auto section_name = QStringLiteral("%1:%2").arg(display_name).arg(dir_name);
            auto index = unhandled_sections.indexOf(section_name);
            if (index != -1)
            {
                // we have to restore this section
                uuids << (*iter);
                unhandled_sections.removeAt(index);
                view_->add_task(display_name, "waiting", 0.0);
            }
        }
    }
    if (!unhandled_sections.isEmpty())
    {
        QString error_message("The following sections were not found: \n");
        for (auto const & section : unhandled_sections)
        {
            error_message += QStringLiteral("\t %1 \n").arg(section);
        }
        view_->print_error_message(error_message);
        exit(1);
    }

    for (auto const & uuid: uuids)
    {
        keeper_client_->enableRestore(uuid, true);
    }
    keeper_client_->startRestore();
    view_->start_printing_tasks();
}

void CommandLineClient::list_backup_sections(keeper::KeeperItemsMap const & choices_values)
{
    QStringList sections;
    for(auto iter = choices_values.begin(); iter != choices_values.end(); ++iter)
    {
        if ((*iter).is_valid() && (*iter).get_type() == "folder")
        {
            sections << (*iter).get_display_name();
        }
    }
    view_->print_sections(sections);
}

void CommandLineClient::list_restore_sections(keeper::KeeperItemsMap const & choices_values)
{
    QMap<QString, QList<keeper::KeeperItem>> values_per_dir;

    for(auto iter = choices_values.begin(); iter != choices_values.end(); ++iter)
    {
        if ((*iter).is_valid() && (*iter).get_type() == "folder")
        {
            auto dir_name = (*iter).get_dir_name();
            if (!dir_name.isEmpty())
            {
                values_per_dir[dir_name].push_back((*iter));
            }
        }
    }

    QStringList sections;
    for(auto iter = values_per_dir.begin(); iter != values_per_dir.end(); ++iter)
    {
        for(auto iter_items = (*iter).begin(); iter_items != (*iter).end(); ++iter_items)
        {
            const auto& values = (*iter_items);
            sections << QStringLiteral("%1:%2").arg(values.get_display_name()).arg(iter.key());
        }
        sections << "";
    }
    view_->print_sections(sections);
}

void CommandLineClient::on_progress_changed()
{
    view_->progress_changed(keeper_client_->progress());
}

void CommandLineClient::on_status_changed()
{
    view_->status_changed(keeper_client_->status());
}

void CommandLineClient::on_keeper_client_finished()
{
    QCoreApplication::processEvents();
    view_->show_info();
    view_->clear_all();
    QCoreApplication::exit(0);
}

bool CommandLineClient::find_choice_value(QVariantMap const & choice, QString const & id, QVariant & value)
{
    auto iter = choice.find(id);
    if (iter == choice.end())
        return false;
    value = (*iter);
    return true;
}

void CommandLineClient::check_for_choices_error(keeper::KeeperError error)
{
    if (error != keeper::KeeperError::OK)
    {
        // an error occurred
        auto error_message = QStringLiteral("Error obtaining keeper choices: %1").arg(view_->get_error_string(error));
        view_->print_error_message(error_message);
        return;
    }
}
