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
 *     Charles Kerr <charles.kerr@canonical.com>
 */

#include "util/connection-helper.h"
#include "storage-framework/storage_framework_client.h"
#include "helper/metadata.h"
#include "service/metadata-provider.h"
#include "service/keeper.h"
#include "service/task-manager.h"

#include <QDebug>
#include <QSharedPointer>
#include <QVector>

#include <algorithm> // std::find_if

class KeeperPrivate
{
public:

    KeeperPrivate(Keeper* keeper,
                  const QSharedPointer<HelperRegistry>& helper_registry,
                  const QSharedPointer<MetadataProvider>& backup_choices,
                  const QSharedPointer<MetadataProvider>& restore_choices)
        : q_ptr(keeper)
        , storage_(new StorageFrameworkClient())
        , helper_registry_(helper_registry)
        , backup_choices_(backup_choices)
        , restore_choices_(restore_choices)
        , task_manager_{helper_registry, storage_}
    {
    }

    ~KeeperPrivate() =default;

    Q_DISABLE_COPY(KeeperPrivate)

    QStringList start_tasks(QStringList const & uuids)
    {
        auto unhandled = QSet<QString>::fromList(uuids);

        auto get_tasks = [](const QVector<Metadata>& pool, QStringList const& keys){
            QMap<QString,Metadata> tasks;
            for (auto const& key : keys) {
                auto it = std::find_if(pool.begin(), pool.end(), [key](Metadata const & m){return m.uuid()==key;});
                if (it != pool.end())
                    tasks[key] = *it;
            }
            return tasks;
        };

        auto tasks = get_tasks(get_backup_choices(), uuids);
        if (!tasks.empty())
        {
            if (task_manager_.start_backup(tasks.values()))
                unhandled.subtract(QSet<QString>::fromList(tasks.keys()));
        }
        else
        {
            tasks = get_tasks(get_restore_choices(), uuids);
            if (!tasks.empty() && task_manager_.start_restore(tasks.values()))
                unhandled.subtract(QSet<QString>::fromList(tasks.keys()));
        }

        if (!unhandled.empty())
            qWarning() << "skipped tasks" << unhandled;

        return QStringList::fromSet(unhandled);
    }

    QVector<Metadata> get_backup_choices() const
    {
        if (cached_backup_choices_.isEmpty())
            cached_backup_choices_ = backup_choices_->get_backups();

        return cached_backup_choices_;
    }

    QVector<Metadata> get_restore_choices() const
    {
        if (cached_restore_choices_.isEmpty())
            cached_restore_choices_ = restore_choices_->get_backups();

        return cached_restore_choices_;
    }

    QVariantDictMap get_state() const
    {
        return task_manager_.get_state();
    }

    void start_backup(quint64 n_bytes,
                      std::function<void(int)>& on_socket_ready)
    {
        qDebug("Keeper::StartBackup(n_bytes=%zu)", size_t(n_bytes));

        // get an uploader
        qDebug() << "Asking for an storage framework socket to the task manager";
        connections_.connect_oneshot(
            &task_manager_,
            &TaskManager::socket_ready,
            on_socket_ready
        );
        task_manager_.ask_for_uploader(n_bytes);
    }

private:

    Keeper * const q_ptr;
    QSharedPointer<StorageFrameworkClient> storage_;
    QSharedPointer<HelperRegistry> helper_registry_;
    QSharedPointer<MetadataProvider> backup_choices_;
    QSharedPointer<MetadataProvider> restore_choices_;
    mutable QVector<Metadata> cached_backup_choices_;
    mutable QVector<Metadata> cached_restore_choices_;
    TaskManager task_manager_;
    ConnectionHelper connections_;
};


Keeper::Keeper(const QSharedPointer<HelperRegistry>& helper_registry,
               const QSharedPointer<MetadataProvider>& backup_choices,
               const QSharedPointer<MetadataProvider>& restore_choices,
               QObject* parent)
    : QObject(parent)
    , d_ptr(new KeeperPrivate(this, helper_registry, backup_choices, restore_choices))
{
}

Keeper::~Keeper() = default;

QStringList
Keeper::start_tasks(QStringList const & uuids)
{
    Q_D(Keeper);

    return d->start_tasks(uuids);
}

void
Keeper::StartBackup(quint64 n_bytes,
                    std::function<void(int)>& on_socket_ready)
{
    Q_D(Keeper);

    return d->start_backup(n_bytes, on_socket_ready);
}

QVector<Metadata>
Keeper::get_backup_choices()
{
    Q_D(Keeper);

    return d->get_backup_choices();
}

QVector<Metadata>
Keeper::get_restore_choices()
{
    Q_D(Keeper);

    return d->get_restore_choices();
}

QVariantDictMap
Keeper::get_state() const
{
    Q_D(const Keeper);

    return d->get_state();
}
