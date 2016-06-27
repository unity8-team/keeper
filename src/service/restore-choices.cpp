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
 *   Charles Kerr <charles.kerr@canoincal.com>
 */

#include "service/restore-choices.h"

#include <unity/storage/common.h>
#include <unity/storage/qt/client/client-api.h>

#include <QDebug>
#include <QtConcurrentRun>

using namespace unity::storage::qt::client;

namespace
{

// FIXME: move to a common file so it can be used by restore-choices and storage_framework_client
Item::SPtr get_backups_folder_sync(const Account::SPtr& account)
{
    Item::SPtr ret;

    // find the root
    // FIXME: returns a container of containers -- do we need to walk them?
    auto roots = account->roots().results();
    if (roots.isEmpty() || roots.front().isEmpty())
    {
        qInfo() << "storage-framework unable to find roots";
        return ret;
    }
    auto root = roots.front().front();
    qDebug() << "id:" << root->native_identity();
    qDebug() << "time:" << root->last_modified_time();

    // look for an Ubuntu Backups subfolder
    // FIXME: i18n?
    const auto backup_dir_name = QString::fromUtf8("Ubuntu Backups");
    try
    {
        auto results = root->lookup(backup_dir_name).results();
        if (!results.empty())
            ret = results.front();
    }
    catch(const std::exception& e)
    {
        qInfo() << backup_dir_name << "lookup returned" << e.what() << "so trying to create";

        auto results = root->create_folder(backup_dir_name).results();
        if (!results.isEmpty())
        {
            qInfo() << backup_dir_name << "created";
            ret = results.front();
        }
    }

    return ret;
}

QFuture<Item::SPtr> get_backups_folder(const Account::SPtr& account)
{
    return QtConcurrent::run([account]{return get_backups_folder_sync(account);});
}

} // namespace



RestoreChoices::RestoreChoices()
    : runtime_(Runtime::create())
{
}

RestoreChoices::~RestoreChoices() =default;

QVector<Metadata>
RestoreChoices::get_backups()
{
    QVector<Metadata> ret;

    // FIXME: blocking
    auto accounts = runtime_->accounts().result();

    // FIXME: blocking
    auto tops = get_backups_folder(accounts.front()).results();
    qInfo() << "tops.results().size()" << tops.size();

    if (!tops.empty())
    {
        auto top = tops.front();
        qInfo() << "top id:" << top->native_identity();
        qInfo() << "top time:" << top->last_modified_time();
    }
  
    // TODO: walk the directory's children

    // TODO: look for a manifest.json in each child subdirectory

    // TODO: parse the manifest.json

    // TODO: how to do this in a nonblocking way but work with the QDBus Adaptor?

    return ret;
}
