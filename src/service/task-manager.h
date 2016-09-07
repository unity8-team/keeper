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
 *   Xavi Garcia <xavi.garcia.mena@canonical.com>
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#pragma once

#include "qdbus-stubs/dbus-types.h"
#include "helper/metadata.h"
#include "keeper-task.h"

#include <QObject>
#include <QVector>

class HelperRegistry;
class TaskManagerPrivate;
class StorageFrameworkClient;

class TaskManager : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(TaskManager)
public:

    TaskManager(QSharedPointer<HelperRegistry> const & helper_registry,
                QSharedPointer<StorageFrameworkClient> const & storage,
                QObject *parent = nullptr);

    virtual ~TaskManager();

    Q_DISABLE_COPY(TaskManager)

    void start_backup(QVector<Metadata> const& tasks);
    void start_restore(QVector<Metadata> const& tasks);

    QVariantDictMap get_state() const;

    void ask_for_uploader(quint64 n_bytes);

Q_SIGNALS:
    void socket_ready(int reply);

private:
    QScopedPointer<TaskManagerPrivate> const d_ptr;
};
