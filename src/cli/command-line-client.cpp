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
    QMap<QString, QVariantMap> choices_values;
    if(!remote)
    {
        choices_values = keeper_client_->getBackupChoices();
        list_backup_sections(choices_values);
    }
    else
    {
        choices_values = keeper_client_->getRestoreChoices();
        list_restore_sections(choices_values);
    }
}

void CommandLineClient::run_backup(QStringList & sections)
{
    auto unhandled_sections = sections;
    auto choices_values = keeper_client_->getBackupChoices();
    QStringList uuids;
    for(auto iter = choices_values.begin(); iter != choices_values.end() && unhandled_sections.size(); ++iter)
    {
        const auto& values = iter.value();
        auto iter_values = values.find("type");
        if (iter_values != values.end())
        {
            if (iter_values.value() == "folder")
            {
                auto iter_display_name = values.find("display-name");
                if (iter_display_name != values.end())
                {
                    auto index = unhandled_sections.indexOf((*iter_display_name).toString());
                    if (index != -1)
                    {
                        // we have to backup this section
                        uuids << iter.key();
                        unhandled_sections.removeAt(index);
                        view_->add_task((*iter_display_name).toString(), "waiting", 0.0);
                    }
                }
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
    auto choices_values = keeper_client_->getRestoreChoices();
    QStringList uuids;
    for(auto iter = choices_values.begin(); iter != choices_values.end(); ++iter)
    {
        const auto& values = iter.value();

        QVariant choice_value;
        auto has_type = find_choice_value(values, "type", choice_value);
        if (has_type && choice_value == "folder")
        {
            QVariant display_name;
            auto has_display_name = find_choice_value(values, "display-name", display_name);
            if (!has_display_name)
                continue;

            QVariant dir_name;
            auto has_dir_name = find_choice_value(values, "dir-name", dir_name);
            if (!has_dir_name)
                continue;

            auto section_name = QStringLiteral("%1:%2").arg(display_name.toString()).arg(dir_name.toString());
            auto index = unhandled_sections.indexOf(section_name);
            if (index != -1)
            {
                // we have to restore this section
                uuids << iter.key();
                unhandled_sections.removeAt(index);
                view_->add_task(display_name.toString(), "waiting", 0.0);
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

void CommandLineClient::list_backup_sections(QMap<QString, QVariantMap> const & choices_values)
{
    QStringList sections;
    for(auto iter = choices_values.begin(); iter != choices_values.end(); ++iter)
    {
        const auto& values = iter.value();

        QVariant choice_value;
        auto has_type = find_choice_value(values, "type", choice_value);
        if (has_type && choice_value == "folder")
        {
            auto has_display_name = find_choice_value(values, "display-name", choice_value);
            if (has_display_name)
            {
                 sections << choice_value.toString();
            }
        }
    }
    view_->print_sections(sections);
}

void CommandLineClient::list_restore_sections(QMap<QString, QVariantMap> const & choices_values)
{
    QMap<QString, QList<QVariantMap>> values_per_dir;

    for(auto iter = choices_values.begin(); iter != choices_values.end(); ++iter)
    {
        const auto& values = iter.value();

        QVariant choice_value;
        auto has_type = find_choice_value(values, "type", choice_value);
        if (has_type && choice_value == "folder")
        {
            auto has_dir_name = find_choice_value(values, "dir-name", choice_value);
            if (!has_dir_name)
                continue;
            values_per_dir[choice_value.toString()].push_back(values);
        }
    }

    QStringList sections;
    for(auto iter = values_per_dir.begin(); iter != values_per_dir.end(); ++iter)
    {
        for(auto iter_items = (*iter).begin(); iter_items != (*iter).end(); ++iter_items)
        {
            const auto& values = (*iter_items);

            QVariant choice_value;
            auto has_type = find_choice_value(values, "type", choice_value);
            if (has_type && choice_value == "folder")
            {
                auto has_display_name = find_choice_value(values, "display-name", choice_value);
                if (has_display_name)
                {
                    sections << QStringLiteral("%1:%2").arg(choice_value.toString()).arg(iter.key());
                }
            }
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
