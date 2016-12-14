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
#include "command-line-client-view.h"

#include <client/client.h>

#include <iostream>
#include <iomanip>

CommandLineClientView::CommandLineClientView(QObject * parent)
    : QObject(parent)
{
    connect(&timer_status_, &QTimer::timeout, this, &CommandLineClientView::show_info);

    // TODO see if we can do this in a better way
    // This line is for the global progress status and percentage
    std::cout << std::endl;
}

void CommandLineClientView::progress_changed(double percentage)
{
    percentage_ = percentage * 100;
}

void CommandLineClientView::status_changed(QString const & status)
{
    if (status_ != status)
    {
        status_ = status;
    }
}

void CommandLineClientView::add_task(QString const & display_name, QString const & initial_status, double initial_percentage)
{
    tasks_strings_[display_name] = get_task_string(display_name, initial_status, initial_percentage, keeper::KeeperError::OK);
    // TODO see if we can do this in a better way
    // We add a line per each backup task
    std::cout << std::endl;
}

void CommandLineClientView::clear_all_tasks()
{
    tasks_strings_.clear();
}

void CommandLineClientView::start_printing_tasks()
{
    timer_status_.start(300);
}

void CommandLineClientView::clear_all()
{
    timer_status_.stop();
    std::cout << std::endl;
}

void CommandLineClientView::print_sections(QStringList const & sections)
{
    for (auto const & section : sections)
    {
        std::cout << section.toStdString() << std::endl;
    }
}

void CommandLineClientView::print_error_message(QString const & error_message)
{
    std::cerr << error_message.toStdString() << std::endl;
}

void CommandLineClientView::show_info()
{
    // TODO Revisit this code to see if we can do this in a different way
    // Maybe using ncurses?

    // Rewind to the beginning
    std::cout << '\r' << std::flush;
    // For every backup task we go up 1 line
    for (auto i = 0; i < tasks_strings_.size(); ++i)
    {
        std::cout << "\e[A";
    }
    // print the tasks
    for (auto iter = tasks_strings_.begin(); iter != tasks_strings_.end(); ++iter)
    {
        std::cout << (*iter).toStdString() << std::setfill(' ') << std::endl;
    }
    std::cout << '\r' << std::fixed << std::setw(30) << status_.toStdString()  << std::setprecision(3)
              << std::setfill(' ') << "  " << percentage_ << " %  " << get_next_spin_char() << "       " << std::flush;
}

char CommandLineClientView::get_next_spin_char()
{
    char cursor[4]={'/','-','\\','|'};
    auto ret = cursor[spin_value_];
    spin_value_ = (spin_value_ + 1) % 4;
    return ret;
}

QString CommandLineClientView::get_task_string(QString const & displayName, QString const & status, double percentage, keeper::KeeperError error)
{

    if (error == keeper::KeeperError::OK)
        return QStringLiteral("%1    %2 %    %3").arg(displayName, 15).arg((percentage * 100), 10, 'f', 2, ' ').arg(status, -15);
    else
        return QStringLiteral("%1    %2 %    %3 %4").arg(displayName, 15).arg((percentage * 100), 10, 'f', 2, ' ').arg(status, -15).arg(get_error_string(error));
}

QString CommandLineClientView::get_error_string(keeper::KeeperError error)
{
    QString ret;
    switch(error)
    {
        case keeper::KeeperError::ERROR_UNKNOWN:
            ret = QStringLiteral("Unknown error");
            break;
        case keeper::KeeperError::HELPER_BAD_URL:
            ret = QStringLiteral("Bad URL for keeper helper");
            break;
        case keeper::KeeperError::HELPER_INACTIVITY_DETECTED:
            ret = QStringLiteral("Inactivity detected in task");
            break;
        case keeper::KeeperError::HELPER_MAX_TIME_WAITING_FOR_START:
            ret = QStringLiteral("Task failed to start");
            break;
        case keeper::KeeperError::HELPER_READ_ERROR:
            ret = QStringLiteral("Read error");
            break;
        case keeper::KeeperError::HELPER_SOCKET_ERROR:
            ret = QStringLiteral("Error creating internal socket");
            break;
        case keeper::KeeperError::HELPER_WRITE_ERROR:
            ret = QStringLiteral("Write error");
            break;
        case keeper::KeeperError::MANIFEST_STORAGE_ERROR:
            ret = QStringLiteral("Error storing manifest file");
            break;
        case keeper::KeeperError::NO_HELPER_INFORMATION_IN_REGISTRY:
            ret = QStringLiteral("No helper information in registry");
            break;
        case keeper::KeeperError::OK:
            ret = QStringLiteral("Success");
            break;
        case keeper::KeeperError::COMMITTING_DATA_ERROR:
            ret = QStringLiteral("Error uploading data");
            break;
    }
    return ret;
}

void CommandLineClientView::on_task_state_changed(QString const & displayName, QString const & status, double percentage, keeper::KeeperError error)
{
    auto iter = tasks_strings_.find(displayName);
    if (iter != tasks_strings_.end())
    {
        tasks_strings_[displayName] = get_task_string(displayName, status, percentage, error);
    }
}
