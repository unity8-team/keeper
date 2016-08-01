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
#include "service/metadata.h"
#include "service/metadata-provider.h"
#include "service/keeper.h"
#include "storage-framework/storage_framework_client.h"

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
public:

    Keeper * const q_ptr;
    QSharedPointer<MetadataProvider> backup_choices_;
    QSharedPointer<MetadataProvider> restore_choices_;
    QScopedPointer<BackupHelper> backup_helper_;
    QScopedPointer<StorageFrameworkClient> storage_;
    QVector<Metadata> cached_backup_choices_;
    QVector<Metadata> cached_restore_choices_;

    KeeperPrivate(Keeper* keeper,
                  const QSharedPointer<MetadataProvider>& backup_choices,
                  const QSharedPointer<MetadataProvider>& restore_choices)
        : q_ptr(keeper)
        , backup_choices_(backup_choices)
        , restore_choices_(restore_choices)
        , backup_helper_(new BackupHelper(DEKKO_APP_ID))
        , storage_(new StorageFrameworkClient())
        , cached_backup_choices_()
        , cached_restore_choices_()
    {
        QObject::connect(storage_.data(), &StorageFrameworkClient::socketClosed, q_ptr, &Keeper::socketClosed);

        // listen for backup helper state changes
        QObject::connect(backup_helper_.data(), &Helper::state_changed,
            std::bind(&KeeperPrivate::on_backup_helper_state_changed, this, std::placeholders::_1)
        );
    }

    ~KeeperPrivate() =default;

    Q_DISABLE_COPY(KeeperPrivate)

    void start_task(QString const &uuid)
    {
        qDebug() << "Starting task: " << uuid;
        auto metadata = get_uuid_metadata(cached_backup_choices_, uuid);
        if (metadata.key() == uuid)
        {
            qDebug() << "Task is a backup task";
            if (!metadata.has_property(Metadata::TYPE_KEY))
            {
                // TODO Report error to user
                qWarning() << "ERROR: uuid: " << uuid << " has no property [" << Metadata::TYPE_KEY << "]";
                return;
            }
            if (metadata.get_property(Metadata::TYPE_KEY).toString() == Metadata::USER_FOLDER_VALUE)
            {
                qDebug() << "Backup for folder type";
                start_folder_type_backup(uuid, metadata);
            }
        }
    }

private:

    void on_backup_helper_state_changed(Helper::State state)
    {
        switch (state)
        {
            case Helper::State::NOT_STARTED:
                break;

            case Helper::State::STARTED:
                qDebug() << "Backup helper started";
                break;

            case Helper::State::CANCELLED:
            case Helper::State::FAILED:
            case Helper::State::COMPLETE:
                qDebug() << "Backup helper finished... closing the socket.";
                storage_->closeUploader();
                break;
        }
    }

    Metadata get_uuid_metadata(QVector<Metadata> const &metadata, QString const & uuid)
    {
        for (auto item : metadata)
        {
            if (item.key() == uuid)
            {
                return item;
            }
        }
        return Metadata();
    }

    void start_folder_type_backup(QString const & uuid, Metadata const & metadata)
    {
        if(!check_for_property(uuid, metadata, Metadata::PATH_KEY))
        {
            // TODO report error to user
            return;
        }
        backup_helper_->set_main_dir_path(metadata.get_property(Metadata::PATH_KEY).toString());
        backup_helper_->set_bin_path(FOLDER_HELPER_BIN_PATH);
        backup_helper_->start();
    }

    bool check_for_property(QString const & uuid, Metadata const & metadata, QString const &key) const
    {
        if (!metadata.has_property(key))
        {
            // TODO Report error to user
            qWarning() << "ERROR: uuid: " << uuid << " has no property [" << key << "]";
            return false;
        }
        return true;
    }
};


Keeper::Keeper(const QSharedPointer<MetadataProvider>& backup_choices,
               const QSharedPointer<MetadataProvider>& restore_choices,
               QObject* parent)
    : QObject(parent)
    , d_ptr(new KeeperPrivate(this, backup_choices, restore_choices))
{
}

Keeper::~Keeper() = default;

void Keeper::start()
{
    Q_D(Keeper);

    qDebug() << "starting backup helper for test";

    d->backup_helper_->start();
}

void Keeper::start_tasks(QStringList const & keys)
{
    Q_D(Keeper);

    // TODO maintain the list of remaining tasks
    if (keys.size())
    {
        d->start_task(keys.at(0));
    }
}

QDBusUnixFileDescriptor
Keeper::StartBackup(QDBusConnection bus, const QDBusMessage& msg, quint64 n_bytes)
{
    Q_D(Keeper);

    qDebug("Helper::StartBackup(n_bytes=%zu)", size_t(n_bytes));

    // the next time we get a socket from storage-framework, return it to our caller
    auto conn = new QMetaObject::Connection();
    auto on_socket_ready = [bus,msg,n_bytes,this,d,conn](int sd) {
        qDebug("getNewFileForBackup() returned socket %d", sd);
        if (sd != -1) {
            qDebug("calling helper.set_storage_framework_socket(n_bytes=%zu socket=%d)", size_t(n_bytes), sd);
            d->backup_helper_->set_expected_size(n_bytes);
            d->backup_helper_->set_storage_framework_socket(sd);
        }
        auto reply = msg.createReply();
        reply << QVariant::fromValue(QDBusUnixFileDescriptor(d->backup_helper_->get_helper_socket()));
        bus.send(reply);
        delete conn;
    };
    // cppcheck-suppress deallocuse
    *conn = QObject::connect(d->storage_.data(), &StorageFrameworkClient::socketReady, on_socket_ready);

    // ask storage framework for a new socket
    d->storage_->getNewFileForBackup(n_bytes);

    // tell the caller that we'll be responding async
    msg.setDelayedReply(true);
    return QDBusUnixFileDescriptor(0);
}

// FOR TESTING PURPOSES ONLY
// we should finish when the helper finishes
void Keeper::finish()
{
    Q_D(Keeper);

    qDebug() << "Closing the socket-------";

    d->storage_->closeUploader();
}

void Keeper::socketClosed()
{
    qDebug() << "The storage framework socket was closed";
}

QVector<Metadata>
Keeper::get_backup_choices()
{
    Q_D(Keeper);

    d->cached_backup_choices_ = d->backup_choices_->get_backups();
    return d->cached_backup_choices_;
}

QVector<Metadata>
Keeper::get_restore_choices()
{
    Q_D(Keeper);

    d->cached_restore_choices_ = d->restore_choices_->get_backups();
    return d->cached_restore_choices_;
}
