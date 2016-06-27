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

using namespace unity::storage::qt::client;

RestoreChoices::RestoreChoices()
    : runtime_(Runtime::create())
{
}

RestoreChoices::~RestoreChoices() =default;

QVector<Metadata>
RestoreChoices::get_backups()
{
    QVector<Metadata> ret;

    // Get the acccounts. (There is only one account for the local client implementation.)
    // We do this synchronously for simplicity.
    auto accounts = runtime_->accounts().result();
    auto root = accounts[0]->roots().result()[0];
    qDebug() << "id:" << root->native_identity();
    qDebug() << "time:" << root->last_modified_time();

    // find the backups dir
    // FIXME: we'll need this again -- in fact StorageFrameworkClient should be using it already
    // so let's extract this into a reusable method that returns a QFuture<Item::SPtr>
    const auto backup_dir_name = QString::fromUtf8("Ubuntu Backups");
    Item::SPtr backup_dir;
    try
    {
        auto looking = root->lookup(backup_dir_name);
        looking.waitForFinished(); // FIXME: blocking
        auto results = looking.results();
        if (!results.empty())
            backup_dir = results.front();
    }
    catch(const std::exception& e)
    {
        qInfo() << backup_dir_name << "lookup returned" << e.what() << "so trying to create";

        auto creating = root->create_folder(backup_dir_name);
        creating.waitForFinished(); // FIXME: blocking
        auto results = creating.results();
        if (!results.isEmpty())
        {
            qInfo() << backup_dir_name << "created";
            backup_dir = results.front();
        }
    }

    if (backup_dir && (backup_dir->type() == unity::storage::ItemType::folder))
    {
        qInfo() << "got directory" << backup_dir->native_identity();
        return ret;
    }

    // TODO: walk the directory's children

    // TODO: look for a manifest.json in each child subdirectory

    // TODO: parse the manifest.json

    // TODO: how to do this in a nonblocking way but work with the QDBus Adaptor?

    return ret;
}
