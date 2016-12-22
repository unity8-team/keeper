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
#include <QDBusMessage>
#include <QDBusConnection>
#include <QSharedPointer>
#include <QVector>

#include <algorithm> // std::find_if
#include <unistd.h>

namespace
{
    QVariantMap strings_to_variants(const QMap<QString,QString>& strings)
    {
        QVariantMap variants;

        for (auto it=strings.begin(), end=strings.end(); it!=end; ++it)
            variants.insert(it.key(), QVariant::fromValue(it.value()));

        return variants;
    }

    keeper::KeeperItemsMap choices_to_variant_dict_map(const QVector<Metadata>& choices)
    {
        keeper::KeeperItemsMap ret;

        for (auto const& metadata : choices)
        {
            keeper::KeeperItem value(strings_to_variants(metadata.get_public_properties()));
            ret.insert(metadata.uuid(), value);
        }

        return ret;
    }
}

class KeeperPrivate : public QObject
{
    Q_OBJECT
public:

    KeeperPrivate(Keeper* keeper,
                  const QSharedPointer<HelperRegistry>& helper_registry,
                  const QSharedPointer<MetadataProvider>& backup_choices,
                  const QSharedPointer<MetadataProvider>& restore_choices,
                  QObject *parent = nullptr)
        : QObject(parent)
        , q_ptr(keeper)
        , storage_(new StorageFrameworkClient())
        , helper_registry_(helper_registry)
        , backup_choices_(backup_choices)
        , restore_choices_(restore_choices)
        , task_manager_{helper_registry, storage_}
    {
        QObject::connect(&task_manager_, &TaskManager::finished,
            std::bind(&KeeperPrivate::on_task_manager_finished, this)
        );
    }

    enum class ChoicesType { BACKUP_CHOICES, RESTORES_CHOICES };

    ~KeeperPrivate() =default;

    Q_DISABLE_COPY(KeeperPrivate)

    void start_tasks(QStringList const & uuids,
                     QDBusConnection bus,
                     QDBusMessage const & msg)
    {
        auto get_tasks = [](const QVector<Metadata>& pool, QStringList const& keys){
            QMap<QString,Metadata> tasks;
            for (auto const& key : keys) {
                auto it = std::find_if(pool.begin(), pool.end(), [key](Metadata const & m){return m.uuid()==key;});
                if (it != pool.end())
                    tasks[key] = *it;
            }
            return tasks;
        };

        // async part
        qDebug() << "Looking for backup options....";
        connections_.connect_oneshot(
            this,
            &KeeperPrivate::backup_choices_ready,
            std::function<void()>{[this, uuids, msg, bus, get_tasks](){
                auto tasks = get_tasks(cached_backup_choices_, uuids);
                if (!tasks.empty())
                {
                    auto unhandled = QSet<QString>::fromList(uuids);
                    if (task_manager_.start_backup(tasks.values()))
                        unhandled.subtract(QSet<QString>::fromList(tasks.keys()));

                    check_for_unhandled_tasks_and_reply(unhandled, bus, msg);
                }
                else // restore
                {
                    qDebug() << "Looking for restore options....";
                    connections_.connect_oneshot(
                        this,
                        &KeeperPrivate::restore_choices_ready,
                        std::function<void(keeper::KeeperError)>{[this, uuids, msg, bus, get_tasks](keeper::KeeperError error){
                            qDebug() << "Choices ready";
                            auto unhandled = QSet<QString>::fromList(uuids);
                            if (error == keeper::KeeperError::OK)
                            {
                                auto restore_tasks = get_tasks(cached_restore_choices_, uuids);
                                qDebug() << "After getting tasks...";
                                if (!restore_tasks.empty() && task_manager_.start_restore(restore_tasks.values()))
                                    unhandled.subtract(QSet<QString>::fromList(restore_tasks.keys()));
                            }
                            check_for_unhandled_tasks_and_reply(unhandled, bus, msg);
                        }}
                    );
                    get_choices(restore_choices_, KeeperPrivate::ChoicesType::RESTORES_CHOICES);
                }
            }}
        );

        get_choices(backup_choices_, KeeperPrivate::ChoicesType::BACKUP_CHOICES);
        msg.setDelayedReply(true);
    }

    void emit_choices_ready(ChoicesType type, keeper::KeeperError error)
    {
        switch(type)
        {
        case KeeperPrivate::ChoicesType::BACKUP_CHOICES:
            Q_EMIT(backup_choices_ready(error));
            break;
        case KeeperPrivate::ChoicesType::RESTORES_CHOICES:
            Q_EMIT(restore_choices_ready(error));
            break;
        }
    }

    void get_choices(const QSharedPointer<MetadataProvider> & provider, ChoicesType type)
    {
        bool check_empty = (type == KeeperPrivate::ChoicesType::BACKUP_CHOICES)
                            ? cached_backup_choices_.isEmpty() : cached_restore_choices_.isEmpty();
        if (check_empty)
        {
            connections_.connect_oneshot(
                provider.data(),
                &MetadataProvider::finished,
                std::function<void(keeper::KeeperError)>{[this, provider, type](keeper::KeeperError error){
                    qDebug() << "Get choices finished";
                    if (error == keeper::KeeperError::OK)
                    {
                        switch (type)
                        {
                        case KeeperPrivate::ChoicesType::BACKUP_CHOICES:
                            cached_backup_choices_ = provider->get_backups();
                            break;
                        case KeeperPrivate::ChoicesType::RESTORES_CHOICES:
                            cached_restore_choices_ = provider->get_backups();
                            break;
                        }
                    }
                    emit_choices_ready(type, error);
                }}
            );
            provider->get_backups_async();
        }
        else
        {
            emit_choices_ready(type, keeper::KeeperError::OK);
        }
    }

    keeper::KeeperItemsMap get_backup_choices_var_dict_map(QDBusConnection bus,
                                                    QDBusMessage const & msg)
    {
        connections_.connect_oneshot(
            this,
            &KeeperPrivate::backup_choices_ready,
            std::function<void(keeper::KeeperError)>{[this, msg, bus](keeper::KeeperError error){
                qDebug() << "Backup choices are ready";
                if (error == keeper::KeeperError::OK)
                {
                    // reply now to the dbus call
                    auto reply = msg.createReply();
                    reply << QVariant::fromValue(choices_to_variant_dict_map(cached_backup_choices_));
                    bus.send(reply);
                }
                else
                {
                    auto message = QStringLiteral("Error obtaining backup choices, keeper returned error: %1").arg(static_cast<int>(error));
                    qWarning() << message;
                    auto reply = msg.createErrorReply(QDBusError::Failed, message);
                    reply << QVariant::fromValue(error);
                    bus.send(reply);
                }
            }}
        );
        get_choices(backup_choices_, KeeperPrivate::ChoicesType::BACKUP_CHOICES);
        msg.setDelayedReply(true);
        return keeper::KeeperItemsMap();
    }

    keeper::KeeperItemsMap get_restore_choices_var_dict_map(QDBusConnection bus,
                                                     QDBusMessage const & msg)
    {
        qDebug() << "Getting restores  --------------------------------";
        cached_restore_choices_.clear();
        connections_.connect_oneshot(
            this,
            &KeeperPrivate::restore_choices_ready,
            std::function<void(keeper::KeeperError)>{[this, msg, bus](keeper::KeeperError error){
                qDebug() << "Restore choices are ready";
                if (error == keeper::KeeperError::OK)
                {
                    // reply now to the dbus call
                    auto reply = msg.createReply();
                    reply << QVariant::fromValue(choices_to_variant_dict_map(cached_restore_choices_));
                    bus.send(reply);
                }
                else
                {
                    auto message = QStringLiteral("Error obtaining restore choices, keeper returned error: %1").arg(static_cast<int>(error));
                    qWarning() << message;
                    auto reply = msg.createErrorReply(QDBusError::Failed, message);
                    reply << QVariant::fromValue(error);
                    bus.send(reply);
                }
            }}
        );
        get_choices(restore_choices_, KeeperPrivate::ChoicesType::RESTORES_CHOICES);
        msg.setDelayedReply(true);
        return keeper::KeeperItemsMap();
    }

    keeper::KeeperItemsMap get_state() const
    {
        return task_manager_.get_state();
    }

    QDBusUnixFileDescriptor start_backup(QDBusConnection bus,
                                         QDBusMessage const & msg,
                                         quint64 n_bytes)
    {
        qDebug("Keeper::StartBackup(n_bytes=%zu)", size_t(n_bytes));

        connections_.connect_oneshot(
            &task_manager_,
            &TaskManager::socket_ready,
            std::function<void(int)>{
                [bus,msg](int fd){
                    qDebug("BackupManager returned socket %d", fd);
                    auto reply = msg.createReply();
                    reply << QVariant::fromValue(QDBusUnixFileDescriptor(fd));
                    bus.send(reply);
                }
            }
        );

        connections_.connect_oneshot(
            &task_manager_,
            &TaskManager::socket_error,
            std::function<void(keeper::KeeperError)>{
                [bus,msg](keeper::KeeperError error){
                    qDebug("BackupManager returned socket error: %d", static_cast<int>(error));
                    bus.send(msg.createErrorReply(QDBusError::InvalidArgs, "Error obtaining remote backup socket"));
                }
            }
        );

        qDebug() << "Asking for a storage framework socket from the task manager";
        task_manager_.ask_for_uploader(n_bytes);

        // tell the caller that we'll be responding async
        msg.setDelayedReply(true);
        return QDBusUnixFileDescriptor(0);
    }


    QDBusUnixFileDescriptor start_restore(QDBusConnection bus,
                                          QDBusMessage const & msg)
    {
        qDebug() << "Keeper::StartRestore()";

        connections_.connect_oneshot(
            &task_manager_,
            &TaskManager::socket_ready,
            std::function<void(int)>{
                [bus,msg](int fd){
                    qDebug("RestoreManager returned socket %d", fd);
                    auto reply = msg.createReply();
                    reply << QVariant::fromValue(QDBusUnixFileDescriptor(fd));
                    close(fd);
                    bus.send(reply);
                }
            }
        );

        connections_.connect_oneshot(
            &task_manager_,
            &TaskManager::socket_error,
            std::function<void(keeper::KeeperError)>{
                [bus,msg](keeper::KeeperError error){
                    qDebug("RestoreManager returned socket error: %d", static_cast<int>(error));
                    bus.send(msg.createErrorReply(QDBusError::InvalidArgs, "Error obtaining remote restore socket"));
                }
            }
        );

        qDebug() << "Asking for a storage framework socket from the task manager";
        task_manager_.ask_for_downloader();

        // tell the caller that we'll be responding async
        msg.setDelayedReply(true);
        return QDBusUnixFileDescriptor(0);
    }

    void cancel()
    {
        task_manager_.cancel();
    }

    void invalidate_choices_cache()
    {
        cached_backup_choices_.clear();
    }

Q_SIGNALS:
    void backup_choices_ready(keeper::KeeperError error);
    void restore_choices_ready(keeper::KeeperError error);

private:
    void on_task_manager_finished()
    {
        // force a backup choices regeneration to avoid repeating uuids
        // between backups
        invalidate_choices_cache();
    }

    void check_for_unhandled_tasks_and_reply(QSet<QString> const & unhandled,
                                   QDBusConnection bus,
                                   QDBusMessage const & msg )
    {
        if (!unhandled.empty())
        {
            qWarning() << "skipped tasks" << unhandled;
            QString text = QStringLiteral("unhandled uuids:");
            for (auto const& uuid : unhandled)
                text += ' ' + uuid;
            bus.send(msg.createErrorReply(QDBusError::InvalidArgs, text));
        }

        auto reply = msg.createReply();
        bus.send(reply);
    }

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

void
Keeper::start_tasks(QStringList const & uuids,
                    QDBusConnection bus,
                    QDBusMessage const & msg)
{
    Q_D(Keeper);

    d->start_tasks(uuids, bus, msg);
}

QDBusUnixFileDescriptor
Keeper::StartBackup(QDBusConnection bus,
                    QDBusMessage const & msg,
                    quint64 n_bytes)
{
    Q_D(Keeper);

    return d->start_backup(bus, msg, n_bytes);
}

QDBusUnixFileDescriptor
Keeper::StartRestore(QDBusConnection bus,
                     QDBusMessage const & msg)
{
    Q_D(Keeper);

    return d->start_restore(bus, msg);
}

keeper::KeeperItemsMap
Keeper::get_backup_choices_var_dict_map(QDBusConnection bus,
                                        QDBusMessage const & msg)
{
    Q_D(Keeper);

    return d->get_backup_choices_var_dict_map(bus, msg);
}

keeper::KeeperItemsMap
Keeper::get_restore_choices(QDBusConnection bus,
                            QDBusMessage const & msg)
{
    Q_D(Keeper);

    return d->get_restore_choices_var_dict_map(bus,msg);
}

keeper::KeeperItemsMap
Keeper::get_state() const
{
    Q_D(const Keeper);

    return d->get_state();
}

void
Keeper::cancel()
{
    Q_D(Keeper);

    return d->cancel();
}

void
Keeper::invalidate_choices_cache()
{
    Q_D(Keeper);

    d->invalidate_choices_cache();
}

#include "keeper.moc"
