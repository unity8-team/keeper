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
 * Author: Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#pragma once

#include <QDBusContext>
#include <QDBusUnixFileDescriptor>
#include <QObject>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QString>
#include <QVector>

class Metadata;
class MetadataProvider;

class KeeperPrivate;
class Keeper : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Keeper)

public:
    Q_DISABLE_COPY(Keeper)

    Keeper(const QSharedPointer<MetadataProvider>& possible,
           const QSharedPointer<MetadataProvider>& available,
           QObject* parent = nullptr);

    virtual ~Keeper();

    QVector<Metadata> get_backup_choices();
    QVector<Metadata> get_restore_choices();

    QDBusUnixFileDescriptor StartBackup(QDBusConnection, const QDBusMessage& message, quint64 nbytes);

public Q_SLOTS:
    // FOR TESTING PURPOSES ONLY
    // we should finish when the helper finishes
    void start();
    void finish();

private:
    QScopedPointer<KeeperPrivate> const d_ptr;
};
