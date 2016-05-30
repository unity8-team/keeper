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
 *     Charles Kerr <charles.kerr@canonical.com>
 */

#include <storage/internal/local-storage.h>

#include <QDir>
#include <QStandardPaths>
#include <QtDebug>

namespace storage
{
namespace internal
{

class LocalStorage::Impl
{
public:

    Impl()
    {
        initBackupsDir();
    }

    ~Impl()
    {
    }

    QVector<BackupInfo>
    getBackups()
    {
        return QVector<BackupInfo>();
    }

    QSharedPointer<QIODevice>
    startRestore(const QString& /*uuid*/)
    {
        return QSharedPointer<QIODevice>();
    }

    QSharedPointer<QIODevice>
    startBackup(const BackupInfo& /*info*/)
    {
        return QSharedPointer<QIODevice>();
    }

private:

    void
    initBackupsDir()
    {
        static constexpr char const * DIRNAME {"Ubuntu Backups"};

        auto path = QStandardPaths::locate(QStandardPaths::AppDataLocation,
                                           DIRNAME,
                                           QStandardPaths::LocateDirectory);
        if (path.isEmpty())
        {
            QDir parent(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
            parent.mkpath(DIRNAME);
            path = parent.absoluteFilePath(DIRNAME);
        }

        QDir dir(path);
        if (!dir.exists())
            qCritical() << path << " should exist but doesn't.";

        backupsDir_ = dir;
    }

    QDir backupsDir_ {};
};

/***
****
***/

LocalStorage::LocalStorage():
    impl_{new Impl{}}
{
}

LocalStorage::~LocalStorage()
{
}

QVector<BackupInfo>
LocalStorage::getBackups()
{
    return impl_->getBackups();
}

QSharedPointer<QIODevice>
LocalStorage::startRestore(const QString& uuid)
{
    return impl_->startRestore(uuid);
}

QSharedPointer<QIODevice>
LocalStorage::startBackup(const BackupInfo& info)
{
    return impl_->startBackup(info);
}

} // namespace internal

} // namespace storage
