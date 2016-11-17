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

#include "qdbus-stubs/dbus-types.h"

#include <unity/storage/qt/client/client-api.h>

#include <QDBusContext>
#include <QDBusUnixFileDescriptor>
#include <QObject>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QString>
#include <QVector>

#include <memory> // sd::shared_ptr

class HelperRegistry;
class Metadata;
class MetadataProvider;

class KeeperPrivate;
class Keeper : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Keeper)

public:
    Q_DISABLE_COPY(Keeper)

    Keeper(const QSharedPointer<HelperRegistry>& helper_registry,
           const QSharedPointer<MetadataProvider>& possible,
           const QSharedPointer<MetadataProvider>& available,
           QObject* parent = nullptr);

    virtual ~Keeper();

    QVariantDictMap get_backup_choices_var_dict_map(QDBusConnection bus, QDBusMessage const & msg);
    QVariantDictMap get_restore_choices(QDBusConnection bus, QDBusMessage const & msg);

    QDBusUnixFileDescriptor StartBackup(QDBusConnection,
                                        QDBusMessage const & message,
                                        quint64 nbytes);


    QDBusUnixFileDescriptor StartRestore(QDBusConnection,
                                        QDBusMessage const & message);

    void RestoreReady();

    void start_tasks(QStringList const & uuids,
                            QDBusConnection bus,
                            QDBusMessage const & msg,
                            bool reset_cached_choices = false);

    QVariantDictMap get_state() const;

    void cancel();

private:
    QScopedPointer<KeeperPrivate> const d_ptr;
};
