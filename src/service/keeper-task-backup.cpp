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

#include "util/connection-helper.h"
#include "storage-framework/storage_framework_client.h"
#include "helper/backup-helper.h"
#include "service/app-const.h" // DEKKO_APP_ID
#include "service/keeper-task-backup.h"
#include "service/keeper-task.h"
#include "service/private/keeper-task_p.h"

class KeeperTaskBackupPrivate : public KeeperTaskPrivate
{
    Q_DECLARE_PUBLIC(KeeperTaskBackup)
public:
    KeeperTaskBackupPrivate(KeeperTask * keeper_task,
                            KeeperTask::TaskData const & task_data,
                            QSharedPointer<HelperRegistry> const & helper_registry,
                            QSharedPointer<StorageFrameworkClient> const & storage)
        : KeeperTaskPrivate(keeper_task, task_data, helper_registry, storage)
    {
    }

    ~KeeperTaskBackupPrivate() = default;

    QStringList get_helper_urls() const
    {
        return helper_registry_->get_backup_helper_urls(task_data_.metadata);
    }

    void init_helper()
    {
        qDebug() << "Initializing a backup helper";
        helper_.reset(new BackupHelper(DEKKO_APP_ID), [](Helper *h){h->deleteLater();});
        qDebug() << "Helper " <<  static_cast<void*>(helper_.data()) << " was created";
    }

    void ask_for_uploader(quint64 n_bytes)
    {
        qDebug() << "asking storage framework for a socket";

        helper_->set_expected_size(n_bytes);

        connections_.connect_future(
            storage_->get_new_uploader(n_bytes),
            std::function<void(std::shared_ptr<Uploader> const&)>{
                [this](std::shared_ptr<Uploader> const& uploader){
                    qDebug("calling helper.set_storage_framework_socket(socket=%d)", int(uploader->socket()->socketDescriptor()));
                    qDebug() << "Helper is " <<  static_cast<void*>(helper_.data());
                    auto backup_helper = qSharedPointerDynamicCast<BackupHelper>(helper_);
                    backup_helper->set_uploader(uploader);
                    Q_EMIT(q_ptr->task_socket_ready(backup_helper->get_helper_socket()));
                }
            }
        );
    }

private:
    ConnectionHelper connections_;
};

KeeperTaskBackup::KeeperTaskBackup(TaskData const & task_data,
           QSharedPointer<HelperRegistry> const & helper_registry,
           QSharedPointer<StorageFrameworkClient> const & storage,
           QObject *parent)
    : KeeperTask(*new KeeperTaskBackupPrivate(this, task_data, helper_registry, storage), parent)
{
}

KeeperTaskBackup::~KeeperTaskBackup() = default;

QStringList KeeperTaskBackup::get_helper_urls() const
{
    Q_D(const KeeperTaskBackup);
    return d->get_helper_urls();
}

void KeeperTaskBackup::init_helper()
{
    Q_D(KeeperTaskBackup);
    d->init_helper();
}

void KeeperTaskBackup::ask_for_uploader(quint64 n_bytes)
{
    Q_D(KeeperTaskBackup);
    d->ask_for_uploader(n_bytes);
}
