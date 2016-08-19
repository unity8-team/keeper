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

#include "app-const.h" // DEKKO_APP_ID
#include "helper/backup-helper.h"
#include "helper/metadata.h"
#include "service/metadata-provider.h"
#include "service/keeper.h"
#include "storage-framework/storage_framework_client.h"
#include "util/dbus-utils.h"

#include <QDebug>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QSharedPointer>
#include <QScopedPointer>
#include <QVariant>
#include <QVector>

#include <uuid/uuid.h>


class KeeperPrivate
{
    enum class TaskType { BACKUP, RESTORE };

    struct TaskData
    {
        QString action;
        QString error;
        TaskType type;
        Metadata metadata;
        float percent_done;
    };

public:

    QScopedPointer<BackupHelper> backup_helper_;
    QScopedPointer<StorageFrameworkClient> storage_;

    KeeperPrivate(Keeper* keeper,
                  const QSharedPointer<HelperRegistry>& helper_registry,
                  const QSharedPointer<MetadataProvider>& backup_choices,
                  const QSharedPointer<MetadataProvider>& restore_choices)
        : backup_helper_(new BackupHelper(DEKKO_APP_ID))
        , storage_(new StorageFrameworkClient())
        , q_ptr(keeper)
        , helper_registry_(helper_registry)
        , backup_choices_(backup_choices)
        , restore_choices_(restore_choices)
    {
        // listen for backup helper state changes
        QObject::connect(backup_helper_.data(), &Helper::state_changed,
            std::bind(&KeeperPrivate::on_helper_state_changed, this, std::placeholders::_1)
        );

        // listen for backup helper process changes
        QObject::connect(backup_helper_.data(), &Helper::percent_done_changed,
            std::bind(&KeeperPrivate::on_helper_percent_done_changed, this, std::placeholders::_1)
        );

        // listen for the storage framework to finish
        QObject::connect(storage_.data(), &StorageFrameworkClient::finished,
            std::bind(&KeeperPrivate::on_storage_framework_finished, this)
        );
    }

    ~KeeperPrivate() =default;

    Q_DISABLE_COPY(KeeperPrivate)

    void start_tasks(QStringList const& uuids)
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
        all_tasks_.clear();
        current_task_.clear();
        remaining_tasks_.clear();
        for(auto const& uuid : uuids)
        {
            Metadata m;
            TaskType type;
            if (!find_task_metadata(uuid, m, type))
            {
                // TODO Report error to user
                qCritical() << "uuid" << uuid << "not found; skipping";
                continue;
            }

            all_tasks_ << uuid;
            remaining_tasks_ << uuid;

            auto& td = task_data_[uuid];
            td.metadata = m;
            td.type = type;
            td.action = QStringLiteral("queued");

            update_task_state(td);
        }

        start_next_task();
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
        return state_;
    }

    QDBusUnixFileDescriptor start_backup(QDBusConnection bus, const QDBusMessage& msg, quint64 n_bytes)
    {
        qDebug("Helper::StartBackup(n_bytes=%zu)", size_t(n_bytes));

        // the next time we get a socket from storage-framework, return it to our caller
        auto tag = new int {};
        auto on_socket_ready = [bus,msg,n_bytes,tag,this](std::shared_ptr<QLocalSocket> const& sf_socket)
        {
            if (sf_socket)
            {
                qDebug("calling helper.set_storage_framework_socket(n_bytes=%zu socket=%d)",
                       size_t(n_bytes),
                       int(sf_socket->socketDescriptor()));
                backup_helper_->set_expected_size(n_bytes);
                backup_helper_->set_storage_framework_socket(sf_socket);
            }
            auto reply = msg.createReply();
            reply << QVariant::fromValue(QDBusUnixFileDescriptor(backup_helper_->get_helper_socket()));
            bus.send(reply);

            // one-shot client, so disconnect now
            disconnect(*tag);
            delete tag;
        };
        // cppcheck-suppress deallocuse
        *tag = remember_connection(
            QObject::connect(
                storage_.data(),
                &StorageFrameworkClient::socketReady,
                on_socket_ready
            )
        );

        // ask storage framework for a new socket
        storage_->getNewFileForBackup(n_bytes);

        // tell the caller that we'll be responding async
        msg.setDelayedReply(true);
        return QDBusUnixFileDescriptor(0);
    }

private:

    void on_helper_state_changed(Helper::State state)
    {
        switch (state)
        {
            case Helper::State::NOT_STARTED:
                break;

            case Helper::State::STARTED:
                qDebug() << "Backup helper started";
                break;

            case Helper::State::CANCELLED:
                set_current_task_action(QStringLiteral("cancelled"));
                qDebug() << "Backup helper cancelled... closing the socket.";
                storage_->finish(false);
                break;

            case Helper::State::FAILED:
                set_current_task_action(QStringLiteral("failed"));
                qDebug() << "Backup helper failed... closing the socket.";
                storage_->finish(false);
                break;

            case Helper::State::COMPLETE:
                task_data_[current_task_].percent_done = 1;
                set_current_task_action(QStringLiteral("complete"));
                qDebug() << "Backup helper finished... closing the socket.";
                try
                {
                    storage_->finish(true);
                }
                catch (std::exception & e)
                {
                    qDebug() << "Failed finishing sf... seting the state to failed";
                    set_current_task_action(QStringLiteral("failed"));
                }
                break;
        }
    }

    void on_helper_percent_done_changed(float /*percent_done*/)
    {
        update_task_state(current_task_);
    }

    void on_storage_framework_finished()
    {
        qDebug() << "storage framework has finished for the current task";
        start_next_task();
    }

    /***
    ****  Task Queueing
    ***/

    void start_next_task()
    {
        bool started {false};

        while (!started && !remaining_tasks_.isEmpty())
            started = start_task(remaining_tasks_.takeFirst());

        if (!started)
            clear_current_task();
    }

    bool start_task(QString const& uuid)
    {
        if (!task_data_.contains(uuid))
        {
            qCritical() << "no task data found for" << uuid;
            return false;
        }

        auto& td = task_data_[uuid];
        if (td.type == TaskType::BACKUP)
        {
            qDebug() << "Task is a backup task";

            const auto urls = helper_registry_->get_backup_helper_urls(td.metadata);
            if (urls.isEmpty())
            {
                td.action = "failed";
                td.error = "no helper information in registry";
                qWarning() << "ERROR: uuid: " << uuid << " has no url";
                return false;
            }

            set_current_task(uuid);
            set_current_task_action(QStringLiteral("saving"));
            backup_helper_->start(urls);
            return true;
        }
        else // RESTORE
        {
            set_current_task(uuid);
            set_current_task_action(QStringLiteral("restoring"));
            qWarning() << "restore not implemented yet";
            return false;
        }
    }

    /***
    ****  State
    ***/

    void update_task_state(QString const& uuid)
    {
        auto it = task_data_.find(uuid);
        if (it == task_data_.end())
        {
            qCritical() << "no task data for" << uuid;
            return;
        }

        update_task_state(it.value());
    }

    void update_task_state(TaskData& td)
    {
        state_[td.metadata.uuid()] = calculate_task_state(td);

#if 0
        // FIXME: we don't need this to work correctly for the sprint because Marcus is polling in a loop
        // but we will need this in order for him to stop doing that

        // TODO: compare old and new and decide if it's worth emitting a PropertyChanged signal;
        // eg don't contribute to dbus noise for minor speed fluctuations

        // TODO: this function is called inside a loop when initializing the state
        // after start_tasks(), so also ensure we don't have a notify flood here

        DBusUtils::notifyPropertyChanged(
            QDBusConnection::sessionBus(),
            *q_ptr,
            DBusTypes::KEEPER_USER_PATH,
            DBusTypes::KEEPER_USER_INTERFACE,
            QStringList(QStringLiteral("State"))
        );
#endif
    }

    QVariantMap calculate_task_state(TaskData& td) const
    {
        QVariantMap ret;

        auto const uuid = td.metadata.uuid();
        bool const current = uuid == current_task_;

        ret.insert(QStringLiteral("action"), td.action);

        ret.insert(QStringLiteral("display-name"), td.metadata.display_name());

        // FIXME: assuming backup_helper_ for now...
        int32_t speed {};
        if (current)
            speed = backup_helper_->speed();
        ret.insert(QStringLiteral("speed"), speed);

        if (current)
            td.percent_done = backup_helper_->percent_done();
        ret.insert(QStringLiteral("percent-done"), double(td.percent_done));

        if (td.action == "failed")
            ret.insert(QStringLiteral("error"), td.error);

        ret.insert(QStringLiteral("uuid"), uuid);

        return ret;
    }

    void set_current_task(QString const& uuid)
    {
        auto const prev = current_task_;

        current_task_ = uuid;

        if (!prev.isEmpty())
            update_task_state(prev);

        if (!uuid.isEmpty())
            update_task_state(uuid);
    }

    void clear_current_task()
    {
        set_current_task(QString());
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

    bool find_task_metadata(QString const& uuid, Metadata& setme_task, TaskType& setme_type) const
    {
        for (const auto& c : get_backup_choices()) {
            if (c.uuid() == uuid) {
                setme_task = c;
                setme_type = TaskType::BACKUP;
                return true;
            }
        }
        for (const auto& c : get_restore_choices()) {
            if (c.uuid() == uuid) {
                setme_task = c;
                setme_type = TaskType::RESTORE;
                return true;
            }
        }
        return false;
    }

    /***
    ****
    ***/

    int remember_connection(QMetaObject::Connection conn)
    {
        static int nexttag {1};
        auto tag = nexttag++;
        auto sp = std::shared_ptr<QMetaObject::Connection>(
            new QMetaObject::Connection(conn),
            [](QMetaObject::Connection *c) {
                QObject::disconnect(*c);
            }
        );
        connections_.insert(tag, sp);
        return tag;
    }

    void disconnect(int tag)
    {
        connections_.remove(tag);
    }

    /***
    ****
    ***/

    Keeper * const q_ptr;
    QSharedPointer<HelperRegistry> helper_registry_;
    QSharedPointer<MetadataProvider> backup_choices_;
    QSharedPointer<MetadataProvider> restore_choices_;
    mutable QVector<Metadata> cached_backup_choices_;
    mutable QVector<Metadata> cached_restore_choices_;
    QStringList all_tasks_;
    QStringList remaining_tasks_;
    QString current_task_;
    QVariantDictMap state_;
    QMap<int,std::shared_ptr<QMetaObject::Connection>> connections_;

    mutable QMap<QString,TaskData> task_data_;
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

    return d->start_backup(bus, msg, n_bytes);
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
