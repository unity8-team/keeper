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

#pragma once

namespace storage
{

struct BackupInfo
{
    QString uuid;
    QSet<QString> packages;
    QDateTime timestamp;
};

struct Storage: public QObject
{
    QVector<BackupInfo> getBackups();

    QIODevice startRestore(const QString& uuid);

    QIODevice startBackup(const QString& uuid, const QSet<QString>& packages, const QDateTime& timestamp);
};

class StorageFactory: public QObject
{
    QFuture<Storage> getStorage();
};

}
