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
 *   Charles Kerr <charles.kerr@canoincal.com>
 */

#include "helper/metadata.h"
#include "keeper-task-backup.h"
#include "storage-framework/storage_framework_client.h"
#include "task-manager.h"
#include "util/dbus-utils.h"

class TaskManagerPrivate
{
public:
    TaskManagerPrivate(TaskManager * manager,
                       QSharedPointer<HelperRegistry> const & helper_registry,
                       QSharedPointer<StorageFrameworkClient> const & storage,
                       QVector<Metadata> const & backup_metadata,
                       QVector<Metadata> const & restore_metadata)
        : q_ptr(manager)
        , helper_registry_(helper_registry)
        , backup_metadata_(backup_metadata)
        , restore_metadata_(restore_metadata)
        , storage_(storage)
    {
    }

    ~TaskManagerPrivate() = default;

    /***
    ****  Task Queueing public
    ***/

    void start_tasks(QStringList const & task_uuids)
    {
        if (!remaining_tasks_.isEmpty())
        {
            // FIXME: return a dbus error here
            qWarning() << "keeper is already active";
            return;
        }

        // rebuild the state variables
        state_.clear();
        task_data_.clear();
        current_task_.clear();
        remaining_tasks_.clear();

        for(auto const& uuid : task_uuids)
        {
            Metadata m;
            KeeperTask::TaskType type;
            if (!find_task_metadata(uuid, m, type))
            {
                // TODO Report error to user
                qCritical() << "uuid" << uuid << "not found; skipping";
                continue;
            }

            remaining_tasks_ << uuid;

            auto& td = task_data_[uuid];
            td.metadata = m;
            td.type = type;
            td.action = QStringLiteral("queued"); // TODO i18n

//            update_task_state(td);
        }

        start_next_task();
    }

    /***
     ***  State public
    ***/

    QVariantDictMap get_state() const
    {
        return state_;
    }

    void on_helper_state_changed(Helper::State state)
    {
        qDebug() << "Task State changed";
        auto& td = task_data_[current_task_];
        update_task_state(td);

        switch (state)
        {
            case Helper::State::NOT_STARTED:
            case Helper::State::STARTED:
            case Helper::State::CANCELLED:
            case Helper::State::FAILED:
            case Helper::State::HELPER_FINISHED:
                break;

            case Helper::State::DATA_COMPLETE:
                task_data_[current_task_].percent_done = 1;
                break;

            case Helper::State::COMPLETE:
                qDebug() << "STARTING NEXT TASK ---------------------------------------";
                start_next_task();
                break;
        }
    }

    void ask_for_storage_framework_socket(quint64 n_bytes)
    {
        qDebug() << "Starting backup";
        if (task_)
        {
            auto backup_task_ = qSharedPointerDynamicCast<KeeperTaskBackup>(task_);
            if (!backup_task_)
            {
                qWarning() << "Only backup tasks are allowed to ask for storage framework sockets";
                // TODO Mark this as an error at the current task and move to the next task
                return;
            }
            backup_task_->ask_for_storage_framework_socket(n_bytes);
        }
    }
private:
    /***
    ****  Task Queueing
    ***/

    bool start_task(QString const& uuid)
    {
        if (!task_data_.contains(uuid))
        {
            qCritical() << "no task data found for" << uuid;
            return false;
        }

        auto& td = task_data_[uuid];

        qDebug() << "Creating task for uuid = " << uuid;
        // initialize a new task

        task_.data()->disconnect();
        if (td.type == KeeperTask::TaskType::BACKUP)
        {
            task_.reset(new KeeperTaskBackup(td, helper_registry_, storage_));
        }
        else
        {
            // TODO initialize a Restore task
        }

        qDebug() << "task created: " << state_;

        set_current_task(uuid);

        QObject::connect(task_.data(), &KeeperTask::task_state_changed,
            std::bind(&TaskManagerPrivate::on_helper_state_changed, this, std::placeholders::_1)
        );

        QObject::connect(task_.data(), &KeeperTask::task_socket_ready,
                    std::bind(&TaskManager::socket_ready, q_ptr, std::placeholders::_1)
                );


        return task_->start();
    }

    void set_current_task(QString const& uuid)
    {
        auto const prev = current_task_;

        current_task_ = uuid;

        if (!uuid.isEmpty())
            update_task_state(uuid);
    }

    void clear_current_task()
    {
        set_current_task(QString());
    }

    void start_next_task()
    {
        bool started {false};

        while (!started && !remaining_tasks_.isEmpty())
            started = start_task(remaining_tasks_.takeFirst());

        if (!started)
            clear_current_task();
    }

    /***
    ****  State
    ***/

    void update_task_state(QString const& uuid)
    {
        qDebug() << "Updating state for " << uuid << static_cast<void *>(task_.data());
        auto it = task_data_.find(uuid);
        if (it == task_data_.end())
        {
            qCritical() << "no task data for" << uuid;
            return;
        }

        update_task_state(it.value());
    }

    void update_task_state(KeeperTask::KeeperTask::TaskData& td)
    {
        state_[td.metadata.uuid()] = task_->state();

        DBusUtils::notifyPropertyChanged(
            QDBusConnection::sessionBus(),
            *q_ptr,
            DBusTypes::KEEPER_USER_PATH,
            DBusTypes::KEEPER_USER_INTERFACE,
            QStringList(QStringLiteral("State"))
        );

        if (!state_[td.metadata.uuid()].isEmpty())
        {
            Q_EMIT(q_ptr->state_changed());
        }
    }

    void set_current_task_action(QString const& action)
    {
        auto& td = task_data_[current_task_];
        td.action = action;
        update_task_state(td);
    }

    /***
    ****  Misc
    ***/

    bool find_task_metadata(QString const& uuid, Metadata& setme_task, KeeperTask::TaskType & type) const
    {
        for (const auto& c : backup_metadata_)
        {
            if (c.uuid() == uuid) {
                setme_task = c;
                type = KeeperTask::TaskType::BACKUP;
                return true;
            }
        }
        for (const auto& c : restore_metadata_)
        {
            if (c.uuid() == uuid) {
                setme_task = c;
                type = KeeperTask::TaskType::RESTORE;
                return true;
            }
        }
        return false;
    }

    TaskManager * const q_ptr;
    QSharedPointer<HelperRegistry> helper_registry_;
    QVector<Metadata> backup_metadata_;
    QVector<Metadata> restore_metadata_;
    QSharedPointer<StorageFrameworkClient> storage_;

    QStringList remaining_tasks_;
    QString current_task_;

    QVariantDictMap state_;
    QSharedPointer<KeeperTask> task_;

    mutable QMap<QString,KeeperTask::TaskData> task_data_;
};

/***
****
***/

TaskManager::TaskManager(QSharedPointer<HelperRegistry> const & helper_registry,
                         QSharedPointer<StorageFrameworkClient> const & storage,
                         QVector<Metadata> const & backup_metadata,
                         QVector<Metadata> const & restore_metadata,
                         QObject *parent)
    : QObject(parent)
    , d_ptr(new TaskManagerPrivate(this, helper_registry, storage, backup_metadata, restore_metadata))
{
}

TaskManager::~TaskManager() = default;

void TaskManager::start_tasks(QStringList const & task_uuids)
{
    Q_D(TaskManager);

    d->start_tasks(task_uuids);
}

QVariantDictMap TaskManager::get_state() const
{
    Q_D(const TaskManager);

    return d->get_state();
}

void TaskManager::ask_for_storage_framework_socket(quint64 n_bytes)
{
    Q_D(TaskManager);

    d->ask_for_storage_framework_socket(n_bytes);
}
