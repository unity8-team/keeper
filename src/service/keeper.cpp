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

namespace
{
    QVariantMap strings_to_variants(const QMap<QString,QString>& strings)
    {
        QVariantMap variants;

        for (auto it=strings.begin(), end=strings.end(); it!=end; ++it)
            variants.insert(it.key(), QVariant::fromValue(it.value()));

        return variants;
    }

    QVariantDictMap choices_to_variant_dict_map(const QVector<Metadata>& choices)
    {
        QVariantDictMap ret;

        for (auto const& metadata : choices)
            ret.insert(metadata.uuid(), strings_to_variants(metadata.get_public_properties()));

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
    }

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
        connections_.connect_oneshot(
            this,
            &KeeperPrivate::choices_ready,
            std::function<void()>{[this, uuids, msg, bus, get_tasks](){
                auto tasks = get_tasks(cached_backup_choices_, uuids);
                if (!tasks.empty())
                {
                    auto unhandled = QSet<QString>::fromList(uuids);
                    if (task_manager_.start_backup(tasks.values()))
                        unhandled.subtract(QSet<QString>::fromList(tasks.keys()));

                    check_for_unhandled_tasks(unhandled, bus, msg);

                    // reply now to the dbus call
                    auto reply = msg.createReply();
                    bus.send(reply);
                }
                else // restore
                {
                    connections_.connect_oneshot(
                        this,
                        &KeeperPrivate::choices_ready,
                        std::function<void()>{[this, uuids, msg, bus, get_tasks](){
                            auto unhandled = QSet<QString>::fromList(uuids);
                            auto restore_tasks = get_tasks(cached_restore_choices_, uuids);
                            if (!restore_tasks.empty() && task_manager_.start_restore(restore_tasks.values()))
                                unhandled.subtract(QSet<QString>::fromList(restore_tasks.keys()));

                            check_for_unhandled_tasks(unhandled, bus, msg);

                            // reply now to the dbus call
                            auto reply = msg.createReply();
                            bus.send(reply);
                        }}
                    );
                    get_restore_choices();
                }
            }}
        );

        get_backup_choices();
        msg.setDelayedReply(true);
    }

    void get_backup_choices()
    {
        if (cached_backup_choices_.isEmpty())
        {
            connections_.connect_oneshot(
                backup_choices_.data(),
                &MetadataProvider::finished,
                std::function<void()>{[this](){
                    qDebug() << "Get backups finished";
                    cached_backup_choices_ = backup_choices_->get_backups();
                    Q_EMIT(choices_ready());
                }}
            );

            backup_choices_->get_backups_async();
        }
        else
        {
            Q_EMIT(choices_ready());
        }
    }

    QVariantDictMap get_backup_choices_var_dict_map(QDBusConnection bus,
                                                    QDBusMessage const & msg)
    {
        connections_.connect_oneshot(
            this,
            &KeeperPrivate::choices_ready,
            std::function<void()>{[this, msg, bus](){
                qDebug() << "Backup choices are ready";
                // reply now to the dbus call
                auto reply = msg.createReply();
                reply << QVariant::fromValue(choices_to_variant_dict_map(cached_backup_choices_));
                bus.send(reply);
            }}
        );

        get_backup_choices();
        msg.setDelayedReply(true);
        return QVariantDictMap();
    }

    QVariantDictMap get_restore_choices_var_dict_map(QDBusConnection bus,
                                                     QDBusMessage const & msg)
    {
        connections_.connect_oneshot(
            this,
            &KeeperPrivate::choices_ready,
            std::function<void()>{[this, msg, bus](){
                qDebug() << "Restore choices are ready";
                // reply now to the dbus call
                auto reply = msg.createReply();
                reply << QVariant::fromValue(choices_to_variant_dict_map(cached_restore_choices_));
                bus.send(reply);
            }}
        );

        get_restore_choices();
        msg.setDelayedReply(true);
        return QVariantDictMap();
    }

    // TODO REFACTOR THIS TO USE THE SAME METHOD POR RESTORES AND BACKUPS
    void get_restore_choices()
    {
        if (cached_restore_choices_.isEmpty())
        {
            connections_.connect_oneshot(
                restore_choices_.data(),
                &MetadataProvider::finished,
                std::function<void()>{[this](){
                    qDebug() << "Get restores finished";
                    cached_restore_choices_ = restore_choices_->get_backups();
                    Q_EMIT(choices_ready());
                }}
            );

            restore_choices_->get_backups_async();
        }
        else
        {
            Q_EMIT(choices_ready());
        }
    }

    QVariantDictMap get_state() const
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

        qDebug() << "Asking for an storage framework socket to the task manager";
        task_manager_.ask_for_uploader(n_bytes);

        // tell the caller that we'll be responding async
        msg.setDelayedReply(true);
        return QDBusUnixFileDescriptor(0);
    }

    void cancel()
    {
        task_manager_.cancel();
    }
Q_SIGNALS:
    void choices_ready();

private:

    void check_for_unhandled_tasks(QSet<QString> const & unhandled,
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

QVariantDictMap
Keeper::get_backup_choices_var_dict_map(QDBusConnection bus,
                                        QDBusMessage const & msg)
{
    Q_D(Keeper);

    return d->get_backup_choices_var_dict_map(bus, msg);
}

QVariantDictMap
Keeper::get_restore_choices(QDBusConnection bus,
                            QDBusMessage const & msg)
{
    Q_D(Keeper);

    return d->get_restore_choices_var_dict_map(bus,msg);
}

QVariantDictMap
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

#include "keeper.moc"
