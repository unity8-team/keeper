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
#include "helper/restore-helper.h"
#include "service/app-const.h" // DEKKO_APP_ID
#include "service/keeper-task-restore.h"
#include "service/keeper-task.h"
#include "service/private/keeper-task_p.h"

namespace sf = unity::storage::qt::client;

class KeeperTaskRestorePrivate : public KeeperTaskPrivate
{
    Q_DECLARE_PUBLIC(KeeperTaskRestore)
public:
    KeeperTaskRestorePrivate(KeeperTask * keeper_task,
                             KeeperTask::TaskData & task_data,
                             QSharedPointer<HelperRegistry> const & helper_registry,
                             QSharedPointer<StorageFrameworkClient> const & storage)
        : KeeperTaskPrivate(keeper_task, task_data, helper_registry, storage)
    {
    }

    ~KeeperTaskRestorePrivate() = default;

    QStringList get_helper_urls() const
    {
        return helper_registry_->get_restore_helper_urls(task_data_.metadata);
    }

    void init_helper()
    {
        helper_.reset(new RestoreHelper(DEKKO_APP_ID), [](Helper *h){h->deleteLater();});
        qDebug() << "Helper " <<  static_cast<void*>(helper_.data()) << " was created";
    }

    void ask_for_downloader()
    {
        qDebug() << "asking storage framework for a socket for reading";

        auto file_name = task_data_.metadata.get_file_name();
        if (file_name.isEmpty())
        {
            qWarning() << "ERROR: the restore task does not provide a valid file name to read from.";
            return;
        }

        auto dir_name = task_data_.metadata.get_dir_name();
        if (dir_name.isEmpty())
        {
            qWarning() << "ERROR: the restore task does not provide a valid directory name.";
            return;
        }

        // extract the dir_name.
        connections_.connect_future(
            storage_->get_new_downloader(dir_name, file_name),
            std::function<void(std::shared_ptr<Downloader> const&)>{
                [this](std::shared_ptr<Downloader> const& downloader){
                    auto fd {-1};
                    if (downloader) {
                        auto restore_helper = qSharedPointerDynamicCast<RestoreHelper>(helper_);
                        restore_helper->set_downloader(downloader);
                        fd = restore_helper->get_helper_socket();
                        Q_EMIT(q_ptr->task_socket_ready(fd));
                    }
                    else
                    {
                        error_ = storage_->get_last_error();
                        qDebug("Emitting task_socket_error(error=%d)", static_cast<int>(error_));
                        Q_EMIT(q_ptr->task_socket_error(error_));
                    }
                }
            }
        );
    }

private:
    ConnectionHelper connections_;
};

KeeperTaskRestore::KeeperTaskRestore(TaskData & task_data,
                                     QSharedPointer<HelperRegistry> const & helper_registry,
                                     QSharedPointer<StorageFrameworkClient> const & storage,
                                     QObject *parent)
    : KeeperTask(*new KeeperTaskRestorePrivate(this, task_data, helper_registry, storage), parent)
{
}

KeeperTaskRestore::~KeeperTaskRestore() = default;

QStringList KeeperTaskRestore::get_helper_urls() const
{
    Q_D(const KeeperTaskRestore);

    return d->get_helper_urls();
}

void KeeperTaskRestore::init_helper()
{
    Q_D(KeeperTaskRestore);

    d->init_helper();
}

void KeeperTaskRestore::ask_for_downloader()
{
    Q_D(KeeperTaskRestore);

    d->ask_for_downloader();
}
