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
 *   Xavi Garcia <xavi.garcia.mena@canonical.com>
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#pragma once

#include "util/connection-helper.h"
#include "storage-framework/uploader.h"

#include <unity/storage/qt/client/client-api.h>

#include <QObject>
#include <QFutureWatcher>

#include <cstddef> // int64_t
#include <functional>
#include <memory>

class StorageFrameworkClient final: public QObject
{
    Q_OBJECT

public:

    Q_DISABLE_COPY(StorageFrameworkClient)
    StorageFrameworkClient(QObject *parent = nullptr);
    virtual ~StorageFrameworkClient();

    QFuture<std::shared_ptr<Uploader>> get_new_uploader(int64_t n_bytes);

    static QString const KEEPER_FOLDER;
private:

    void add_accounts_task(std::function<void(QVector<unity::storage::qt::client::Account::SPtr> const&)> task);
    void add_roots_task(std::function<void(QVector<unity::storage::qt::client::Root::SPtr> const&)> task);

    unity::storage::qt::client::Account::SPtr choose(QVector<unity::storage::qt::client::Account::SPtr> const& choices) const;
    unity::storage::qt::client::Root::SPtr choose(QVector<unity::storage::qt::client::Root::SPtr> const& choices) const;

    QFuture<unity::storage::qt::client::Folder::SPtr> create_keeper_folder(unity::storage::qt::client::Root::SPtr const & root);

    unity::storage::qt::client::Runtime::SPtr runtime_;
    ConnectionHelper connection_helper_;
};
