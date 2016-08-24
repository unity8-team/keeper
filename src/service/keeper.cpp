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

#include "helper/metadata.h"
#include "service/metadata-provider.h"
#include "service/keeper.h"
#include "storage-framework/storage_framework_client.h"
#include "task-manager.h"

#include <QDebug>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QSharedPointer>
#include <QVector>

class KeeperPrivate
{
public:
    QScopedPointer<TaskManager> task_manager_;

    KeeperPrivate(Keeper* keeper,
                  const QSharedPointer<HelperRegistry>& helper_registry,
                  const QSharedPointer<MetadataProvider>& backup_choices,
                  const QSharedPointer<MetadataProvider>& restore_choices)
        : q_ptr(keeper)
        , storage_(new StorageFrameworkClient())
        , helper_registry_(helper_registry)
        , backup_choices_(backup_choices)
        , restore_choices_(restore_choices)
    {
        task_manager_.reset(new TaskManager(helper_registry, storage_, get_backup_choices(), get_restore_choices()));
    }

    ~KeeperPrivate() =default;

    Q_DISABLE_COPY(KeeperPrivate)

    void start_tasks(QStringList const& uuids)
    {
        task_manager_->start_tasks(uuids);
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
        return task_manager_->get_state();
    }

private:

    /***
    ****
    ***/

    Keeper * const q_ptr;
    QSharedPointer<StorageFrameworkClient> storage_;
    QSharedPointer<HelperRegistry> helper_registry_;
    QSharedPointer<MetadataProvider> backup_choices_;
    QSharedPointer<MetadataProvider> restore_choices_;
    mutable QVector<Metadata> cached_backup_choices_;
    mutable QVector<Metadata> cached_restore_choices_;
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

void Keeper::start_tasks(QStringList const & keys)
{
    Q_D(Keeper);

    d->start_tasks(keys);
}

QDBusUnixFileDescriptor
Keeper::StartBackup(QDBusConnection bus, const QDBusMessage& msg, quint64 n_bytes)
{
    Q_D(Keeper);

    qDebug("Keeper::StartBackup(n_bytes=%zu)", size_t(n_bytes));

    // the next time we get a socket from storage-framework, return it to our caller
    auto on_socket_ready = [bus,msg,n_bytes,this,d](int backup_reply)
    {
        qDebug("BackupManager returned socket %d", backup_reply);
        auto reply = msg.createReply();
        reply << QVariant::fromValue(QDBusUnixFileDescriptor(backup_reply));
        bus.send(reply);
    };
    // cppcheck-suppress deallocuse
    QObject::connect(d->task_manager_.data(), &TaskManager::socket_ready, on_socket_ready);

    qDebug() << "Calling backup manager->start_backup";
    d->task_manager_->ask_for_storage_framework_socket(n_bytes);

    // tell the caller that we'll be responding async
    msg.setDelayedReply(true);
    return QDBusUnixFileDescriptor(0);
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
