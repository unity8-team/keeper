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
                            KeeperTask::TaskData & task_data,
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
        QObject::connect(helper_.data(), &Helper::error, [this](keeper::KeeperError error){ error_ = error;});
    }

    void ask_for_uploader(quint64 n_bytes, QString const & dir_name)
    {
        qDebug() << "asking storage framework for a socket";

        helper_->set_expected_size(n_bytes);

        const auto file_name = QString("%1.keeper").arg(task_data_.metadata.display_name());

        connections_.connect_future(
            storage_->get_new_uploader(n_bytes, dir_name, file_name),
            std::function<void(std::shared_ptr<Uploader> const&)>{
                [this](std::shared_ptr<Uploader> const& uploader){
                    int fd {-1};
                    if (uploader) {
                        auto backup_helper = qSharedPointerDynamicCast<BackupHelper>(helper_);
                        backup_helper->set_uploader(uploader);
                        fd = backup_helper->get_helper_socket();
                    }
                    qDebug("emitting task_socket_ready(socket=%d)", fd);
                    Q_EMIT(q_ptr->task_socket_ready(fd));
                }
            }
        );
    }

    QString get_file_name() const
    {
        auto backup_helper = qSharedPointerDynamicCast<BackupHelper>(helper_);
        return backup_helper->get_uploader_committed_file_name();
    }

private:
    ConnectionHelper connections_;
    QString file_name_;
};

KeeperTaskBackup::KeeperTaskBackup(TaskData & task_data,
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

void KeeperTaskBackup::ask_for_uploader(quint64 n_bytes, QString const & dir_name)
{
    Q_D(KeeperTaskBackup);

    d->ask_for_uploader(n_bytes, dir_name);
}

QString KeeperTaskBackup::get_file_name() const
{
    Q_D(const KeeperTaskBackup);

    return d->get_file_name();
}
