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

#include "helper/backup-helper.h"
#include "service/metadata.h"
#include "service/metadata-provider.h"
#include "service/keeper.h"
#include "storage-framework/storage_framework_client.h"

#include <QDebug>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QStandardPaths>
#include <QStringList>
#include <QVariantMap>

#include <uuid/uuid.h>

namespace
{
    constexpr char const DEKKO_APP_ID[] = "dekko.dekkoproject_dekko_0.6.20";
}

class KeeperPrivate
{
    Q_DISABLE_COPY(KeeperPrivate)
    Q_DECLARE_PUBLIC(Keeper)

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
    {
        QObject::connect(storage_.data(), &StorageFrameworkClient::socketReady, q_ptr, &Keeper::socketReady);
        QObject::connect(storage_.data(), &StorageFrameworkClient::socketClosed, q_ptr, &Keeper::socketClosed);
        QObject::connect(backup_helper_.data(), &BackupHelper::started, q_ptr, &Keeper::helperStarted);
        QObject::connect(backup_helper_.data(), &BackupHelper::finished, q_ptr, &Keeper::helperFinished);
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

    qDebug() << "Backup start";
    qDebug() << "Waiting for a valid socket from the storage framework";

    d->storage_->getNewFileForBackup();
}

QDBusUnixFileDescriptor Keeper::GetBackupSocketDescriptor()
{
    Q_D(Keeper);

    qDebug() << "Sending the socket " << d->storage_->getUploaderSocketDescriptor();

    return QDBusUnixFileDescriptor(d->storage_->getUploaderSocketDescriptor());
}

// FOR TESTING PURPOSES ONLY
// we should finish when the helper finishes
void Keeper::finish()
{
    Q_D(Keeper);

    qDebug() << "Closing the socket-------";

    d->storage_->closeUploader();
}

void Keeper::socketReady(int sd)
{
    Q_D(Keeper);

    qDebug() << "I've got a new socket: " << sd;
    qDebug() << "Starting the backup helper";

    d->backup_helper_->start(sd);
}

void Keeper::helperStarted()
{
    qDebug() << "Backup helper started";
}

void Keeper::helperFinished()
{
    Q_D(Keeper);

    qDebug() << "Backup helper finished";
    qDebug() << "Closing the socket";
    d->storage_->closeUploader();
}

void Keeper::socketClosed()
{
    qDebug() << "The storage framework socket was closed";
}

QVector<Metadata>
Keeper::GetBackupChoices() const
{
    Q_D(const Keeper);

    d->cached_backup_choices_ = d->backup_choices_->get_backups();
    return d->cached_backup_choices_;
}

QVector<Metadata>
Keeper::GetRestoreChoices() const
{
    Q_D(const Keeper);

    d->cached_restore_choices_ = d->restore_choices_->get_backups();
    return d->cached_restore_choices_;
}
