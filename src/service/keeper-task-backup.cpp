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
 *   Xavi Garcia <xavi.garcia.mena@canoincal.com>
 *   Charles Kerr <charles.kerr@canonical.com>
 */
#include "app-const.h" // DEKKO_APP_ID
#include "helper/backup-helper.h"
#include "keeper-task-backup.h"
#include "keeper-task.h"
#include "private/keeper-task_p.h"
#include "storage-framework/storage_framework_client.h"

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
        storage_framework_socket_connection_ready_.reset(
                  new QMetaObject::Connection(
                       QObject::connect(
                               storage_.data(), &StorageFrameworkClient::socketReady,
                           std::bind(&KeeperTaskBackupPrivate::on_backup_socket_ready, this, std::placeholders::_1)
                       )
                   ),
                    [](QMetaObject::Connection* c){
                        QObject::disconnect(*c);
                    }
        );
    }

    ~KeeperTaskBackupPrivate() = default;

    QStringList get_helper_urls() const
    {
        return helper_registry_->get_backup_helper_urls(task_data_.metadata);
    }

    void init_helper()
    {
        qDebug() << "Initializing a backup helper";
        helper_.reset(new BackupHelper(DEKKO_APP_ID));
        qDebug() << "Helper " <<  static_cast<void*>(helper_.data()) << " was created";

        auto backup_helper = qSharedPointerDynamicCast<BackupHelper>(helper_);
        if (backup_helper)
        {
            // listen for the storage framework to finish
            storage_framework_socket_connection_finished_.reset(
                      new QMetaObject::Connection(
                           QObject::connect(
                                   storage_.data(), &StorageFrameworkClient::finished,
                               std::bind(&BackupHelper::on_storage_framework_finished, backup_helper.data())
                           )
                       ),
                        [](QMetaObject::Connection* c){
                            QObject::disconnect(*c);
                        }
            );
        }
    }

    void on_backup_socket_ready(std::shared_ptr<QLocalSocket> const &  sf_socket)
    {
        qDebug("calling helper.set_storage_framework_socket(socket=%d)", int(sf_socket->socketDescriptor()));
        qDebug() << "Helper is " <<  static_cast<void*>(helper_.data());
        auto backup_helper = qSharedPointerDynamicCast<BackupHelper>(helper_);
        if (!backup_helper)
        {
            qWarning() << "Only backup tasks are allowed to ask for storage framework sockets";
            helper_->stop();
            return;
        }
        backup_helper->set_storage_framework_socket(sf_socket);
        Q_EMIT(q_ptr->task_socket_ready(backup_helper->get_helper_socket()));
    }

    void ask_for_storage_framework_socket(quint64 n_bytes)
    {
        qDebug() << "asking storage framework for a socket";
        storage_->getNewFileForBackup(n_bytes);
        helper_->set_expected_size(n_bytes);
    }

    void on_helper_state_changed(Helper::State state) override
    {
        auto new_state = state;
        switch (state)
        {
            case Helper::State::NOT_STARTED:
                break;

            case Helper::State::STARTED:
                qDebug() << "Backup helper started";
                break;

            case Helper::State::CANCELLED:
                qDebug() << "Backup helper cancelled... closing the socket.";
                storage_->finish(false);
                break;

            case Helper::State::FAILED:
                qDebug() << "Backup helper failed... closing the socket.";
                storage_->finish(false);
                break;

            case Helper::State::HELPER_FINISHED:
                qDebug() << "Backup helper process finished... ";
                break;

            case Helper::State::DATA_COMPLETE:
                task_data_.percent_done = 1;
                qDebug() << "Backup helper finished... closing the socket.";
                try
                {
                    storage_->finish(true);
                }
                catch (std::exception & e)
                {
                    qDebug() << "Failed finishing sf... setting the state to failed";
                    new_state = Helper::State::FAILED;
                }
                break;
            case Helper::State::COMPLETE:
                break;
        }
        KeeperTaskPrivate::on_helper_state_changed(new_state);
    }

private:
    std::shared_ptr<QMetaObject::Connection> storage_framework_socket_connection_ready_;
    std::shared_ptr<QMetaObject::Connection> storage_framework_socket_connection_finished_;
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

void KeeperTaskBackup::ask_for_storage_framework_socket(quint64 n_bytes)
{
    Q_D(KeeperTaskBackup);
    d->ask_for_storage_framework_socket(n_bytes);
}
