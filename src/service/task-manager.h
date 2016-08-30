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
 *   Xavi Garcia <xavi.garcia.mena@canoincal.com>
 *   Charles Kerr <charles.kerr@canoincal.com>
 */

#pragma once

#include "helper/metadata.h"
#include "keeper-task.h"
#include "qdbus-stubs/dbus-types.h"

#include <QObject>

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
                QVector<Metadata> const & backup_metadata,
                QVector<Metadata> const & restore_metadata,
                QObject *parent = nullptr);

    virtual ~TaskManager();

    Q_DISABLE_COPY(TaskManager)

    void start_tasks(QStringList const & task_uuids);

    QVariantDictMap get_state() const;

    void ask_for_storage_framework_socket(quint64 n_bytes);

Q_SIGNALS:
    void socket_ready(int reply);

private:
    QScopedPointer<TaskManagerPrivate> const d_ptr;
};
