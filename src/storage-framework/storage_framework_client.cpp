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

#include "storage-framework/storage_framework_client.h"
#include "storage-framework/sf-uploader.h"

#include <QDateTime>
#include <QVector>
#include <QString>

namespace sf = unity::storage::qt::client;

/***
****
***/

StorageFrameworkClient::StorageFrameworkClient(QObject *parent)
    : QObject(parent)
    , runtime_(sf::Runtime::create())
{
}

StorageFrameworkClient::~StorageFrameworkClient() =default;

/***
****
***/

sf::Account::SPtr
StorageFrameworkClient::choose(QVector<sf::Account::SPtr> const& choices) const
{
    sf::Account::SPtr ret;

    qDebug() << "choosing from" << choices.size() << "accounts";
    if (choices.empty())
    {
        qWarning() << "no storage-framework accounts to pick from";
    }
    else // for now just pick the first one. FIXME
    {
        ret = choices.front();
    }

    return ret;
}

sf::Root::SPtr
StorageFrameworkClient::choose(QVector<sf::Root::SPtr> const& choices) const
{
    sf::Root::SPtr ret;

    qDebug() << "choosing from" << choices.size() << "roots";
    if (choices.empty())
    {
        qWarning() << "no storage-framework roots to pick from";
    }
    else // for now just pick the first one. FIXME
    {
        ret = choices.front();
    }

    return ret;
}

/***
****
***/

void
StorageFrameworkClient::add_accounts_task(std::function<void(QVector<sf::Account::SPtr> const&)> task)
{
    connection_helper_.connect_future(runtime_->accounts(), task);
}

void
StorageFrameworkClient::add_roots_task(std::function<void(QVector<sf::Root::SPtr> const&)> task)
{
    add_accounts_task([this, task](QVector<sf::Account::SPtr> const& accounts)
    {
        auto account = choose(accounts);
        if (account)
            connection_helper_.connect_future(account->roots(), task);
    });
}

QFuture<std::shared_ptr<Uploader>>
StorageFrameworkClient::get_new_uploader(int64_t n_bytes)
{
    QFutureInterface<std::shared_ptr<Uploader>> fi;

    add_roots_task([this, fi, n_bytes](QVector<sf::Root::SPtr> const& roots)
    {
        auto root = choose(roots);
        if (root)
        {
            std::function<void(std::shared_ptr<sf::Uploader> const&)> on_uploader_ready = [this, fi](std::shared_ptr<sf::Uploader> const& uploader)
            {
                qDebug() << "root->create_file() finished";
                auto wrapper = std::shared_ptr<Uploader>(new StorageFrameworkUploader(uploader, this), [](Uploader* u){u->deleteLater();});
                QFutureInterface<decltype(wrapper)> qfi(fi);
                qfi.reportResult(wrapper);
                qfi.reportFinished();
            };

            auto const now = QDateTime::currentDateTime();
            auto const filename = QStringLiteral("Backup_%1").arg(now.toString("dd.MM.yyyy-hh.mm.ss.zzz"));
            connection_helper_.connect_future(root->create_file(filename, n_bytes), on_uploader_ready);
        }
    });

    return fi.future();
}
