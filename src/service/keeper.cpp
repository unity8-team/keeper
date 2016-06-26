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

#include <QDebug>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QStandardPaths>
#include <QStringList>
#include <QVariantMap>

#include <uuid/uuid.h>

#include <map>

#include <helper/backup-helper.h>
#include <storage-framework/storage_framework_client.h>
#include "keeper.h"

namespace
{
    constexpr char const DEKKO_APP_ID[] = "dekko.dekkoproject_dekko_0.6.20";
}

class KeeperPrivate
{
    Q_DISABLE_COPY(KeeperPrivate)
    Q_DECLARE_PUBLIC(Keeper)

    Keeper * const q_ptr;
    std::map<QString,QVariantMap> possible_backups_;
    QScopedPointer<BackupHelper> backup_helper_;
    QScopedPointer<StorageFrameworkClient> storage_;

    explicit KeeperPrivate(Keeper* keeper)
        : q_ptr(keeper)
        , possible_backups_()
        , backup_helper_(new BackupHelper(DEKKO_APP_ID))
        , storage_(new StorageFrameworkClient())
    {
        init_possible_backups();

        QObject::connect(storage_.data(), &StorageFrameworkClient::socketReady, q_ptr, &Keeper::socketReady);
        QObject::connect(storage_.data(), &StorageFrameworkClient::socketClosed, q_ptr, &Keeper::socketClosed);
        QObject::connect(backup_helper_.data(), &BackupHelper::started, q_ptr, &Keeper::helperStarted);
        QObject::connect(backup_helper_.data(), &BackupHelper::finished, q_ptr, &Keeper::helperFinished);
    }

    void init_possible_backups()
    {
        // FIXME: add the click packages here

        // XDG User Directories

        const QStandardPaths::StandardLocation standard_locations[] = {
            QStandardPaths::DocumentsLocation,
            QStandardPaths::MoviesLocation,
            QStandardPaths::PicturesLocation,
            QStandardPaths::MusicLocation
        };

        const auto display_name_str = QString::fromUtf8("display-name");
        const auto path_str = QString::fromUtf8("path");

        for (const auto& sl : standard_locations)
        {
            const auto name = QStandardPaths::displayName(sl);
            const auto locations = QStandardPaths::standardLocations(sl);
            if (locations.empty())
            {
                qWarning() << "unable to find path for"  << name;
            }
            else
            {
                uuid_t keyuu;
                uuid_generate(keyuu);
                char keybuf[37];
                uuid_unparse(keyuu, keybuf);
                const QString keystr = QString::fromUtf8(keybuf);

                auto& pb = possible_backups_[keystr];
                pb[display_name_str] = name;
                pb[path_str] = locations.front();
            }
        }
    }
};


Keeper::Keeper(QObject* parent)
    : QObject(parent)
    , d_ptr(new KeeperPrivate(this))
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

QVector<QPair<QString,QString>>
Keeper::GetPossibleBackups() const
{
    Q_D(const Keeper);

    QVector<QPair<QString,QString>> ret;

    const auto display_name_str = QString::fromUtf8("display-name");

    for(const auto& it : d->possible_backups_)
    {
        const auto& key = it.first;

        const auto nameit = it.second.find(display_name_str);
        if (nameit != it.second.end())
            ret.push_back(qMakePair(key, nameit.value().toString()));
    }

    return ret;
}
