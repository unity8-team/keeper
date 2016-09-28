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
                       QSharedPointer<StorageFrameworkClient> const & storage)
        : q_ptr(manager)
        , helper_registry_(helper_registry)
        , storage_(storage)
    {
    }

    ~TaskManagerPrivate() = default;

    bool start_backup(QList<Metadata> const& tasks)
    {
        return start_tasks(tasks, Mode::BACKUP);
    }

    bool start_restore(QList<Metadata> const& tasks)
    {
        return start_tasks(tasks, Mode::RESTORE);
    }

    /***
     ***  State public
    ***/

    QVariantDictMap get_state() const
    {
        return state_;
    }

    void ask_for_uploader(quint64 n_bytes)
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
            backup_task_->ask_for_uploader(n_bytes);
        }
    }

    void cancel()
    {
        if (task_)
        {
            task_->cancel();
        }
        for (auto task : remaining_tasks_)
        {
            auto& td = task_data_[task];
            td.action = QStringLiteral("cancelled"); // TODO i18n
            set_initial_task_state(td);
        }
        // notify the initial state once for all tasks
        notify_state_changed();
        remaining_tasks_.clear();
    }

private:

    enum class Mode { IDLE, BACKUP, RESTORE };

    bool start_tasks(QList<Metadata> const& tasks, Mode mode)
    {
        bool success = true;

        if (!remaining_tasks_.isEmpty())
        {
            // FIXME: return a dbus error here
            qWarning() << "keeper is already active";
            success = false;
        }
        else
        {
            // rebuild the state variables
            state_.clear();
            task_data_.clear();
            current_task_.clear();
            remaining_tasks_.clear();

            mode_ = mode;

            for(auto const& metadata : tasks)
            {
                auto const uuid = metadata.uuid();

                remaining_tasks_ << uuid;

                auto& td = task_data_[uuid];
                td.metadata = metadata;
                td.action = QStringLiteral("queued"); // TODO i18n
                set_initial_task_state(td);
            }

            // notify the initial state once for all tasks
            notify_state_changed();

            start_next_task();
        }

        return success;
    }

    void on_helper_state_changed(Helper::State state)
    {
        qDebug() << "Task State changed";
        auto& td = task_data_[current_task_];
        update_task_state(td);

        if (state == Helper::State::COMPLETE || state == Helper::State::FAILED)
        {
            qDebug() << "STARTING NEXT TASK ---------------------------------------";
            start_next_task();
        }
    }

    /***
    ****  Task Queueing
    ***/

    bool start_task(QString const& uuid)
    {
        auto it = task_data_.find(uuid);
        if (it == task_data_.end())
        {
            qCritical() << "no task data for" << uuid;
            return false;
        }

        auto& td = it.value();

        qDebug() << "Creating task for uuid = " << uuid;
        // initialize a new task

        task_.data()->disconnect();
        if (mode_ == Mode::BACKUP)
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

    void set_initial_task_state(KeeperTask::KeeperTask::TaskData& td)
    {
        state_[td.metadata.uuid()] = KeeperTask::get_initial_state(td);
    }

    void notify_state_changed()
    {
        DBusUtils::notifyPropertyChanged(
            QDBusConnection::sessionBus(),
            *q_ptr,
            DBusTypes::KEEPER_USER_PATH,
            DBusTypes::KEEPER_USER_INTERFACE,
            QStringList(QStringLiteral("State"))
        );

        Q_EMIT(q_ptr->state_changed());
    }

    void update_task_state(KeeperTask::KeeperTask::TaskData& td)
    {
        auto task_state = task_->state();

        // avoid sending repeated states to minimize the use of the bus
        if (task_state != state_[td.metadata.uuid()] && !task_state.isEmpty())
        {
            state_[td.metadata.uuid()] = task_state;

            // FIXME: we don't need this to work correctly for the sprint because Marcus is polling in a loop
            // but we will need this in order for him to stop doing that

            // TODO: compare old and new and decide if it's worth emitting a PropertyChanged signal;
            // eg don't contribute to dbus noise for minor speed fluctuations
            notify_state_changed();
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

    TaskManager * const q_ptr;
    Mode mode_ {Mode::IDLE};
    QSharedPointer<HelperRegistry> helper_registry_;
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
                         QObject *parent)
    : QObject(parent)
    , d_ptr(new TaskManagerPrivate(this, helper_registry, storage))
{
}

TaskManager::~TaskManager() = default;

bool
TaskManager::start_backup(QList<Metadata> const& tasks)
{
    Q_D(TaskManager);

    return d->start_backup(tasks);
}

bool
TaskManager::start_restore(QList<Metadata> const& tasks)
{
    Q_D(TaskManager);

    return d->start_restore(tasks);
}

QVariantDictMap TaskManager::get_state() const
{
    Q_D(const TaskManager);

    return d->get_state();
}

void TaskManager::ask_for_uploader(quint64 n_bytes)
{
    Q_D(TaskManager);

    d->ask_for_uploader(n_bytes);
}

void TaskManager::cancel()
{
    Q_D(TaskManager);

    d->cancel();
}
