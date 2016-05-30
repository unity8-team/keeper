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
#include <QSharedPointer>
#include <QString>
#include <QVariant>
#include <QVector>

namespace storage
{

// NB: this is an experimental class whose API should and will change
struct BackupInfo
{
    QString uuid;

    // TODO: for 1.0 we only need multimedia and everything else; is a packages list needed?
    QSet<QString> packages;
    QDateTime timestamp;

    // TODO: this is an escape hatch for if we need to add more properties in the future.
    // what are the use cases?
    QVariantMap properties;
};

// NB: this is an experimental class whose API should and will change
// TODO: other than these three functions, is there any other functionality to wrap around the storage framework?
// TODO: these three give Storage too much responsibility, eg startBackup means Storage needs to know how to launch helpers. We want a middleman to decouple them
class Storage
{
public:

    virtual ~Storage();

    Storage(Storage&&) =delete;
    Storage(Storage const&) =delete;
    Storage& operator=(Storage&&) =delete;
    Storage& operator=(Storage const&) =delete;

    virtual QVector<BackupInfo> getBackups() =0;

    virtual QSharedPointer<QIODevice> startRestore(const QString& uuid) =0;

    // TODO: should Storage create the uuid itself?
    virtual QSharedPointer<QIODevice> startBackup(const BackupInfo& info) =0;

protected:

    Storage();
};

}
