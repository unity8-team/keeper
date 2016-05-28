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

#include <QDateTime>
#include <QIODevice>
#include <QSet>
#include <QString>
#include <QVector>

namespace storage
{

struct BackupInfo
{
    QString uuid;
    QSet<QString> packages;
    QDateTime timestamp;
};

// NB: this is very much a work in progress
class Storage
{
public:

    virtual ~Storage();

    Storage(Storage&&) =delete;
    Storage(Storage const&) =delete;
    Storage& operator=(Storage&&) =delete;
    Storage& operator=(Storage const&) =delete;

    virtual QVector<BackupInfo> getBackups() =0;
    virtual QIODevice* startRestore(const QString& uuid) =0;
    virtual QIODevice* startBackup(const QString& uuid, const QSet<QString>& packages, const QDateTime& timestamp) =0;

protected:

    Storage();
};

}
