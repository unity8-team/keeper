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

#include "helper/metadata.h"
#include "helper/backup-helper.h"
#include "helper/helper.h"

#include <QObject>
#include <QSharedPointer>

class HelperRegistry;
class KeeperTaskPrivate;
class StorageFrameworkClient;

class KeeperTask : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(KeeperTask)
public:

    struct TaskData
    {
        QString action;
        KeeperError error;
        Metadata metadata;
    };

    KeeperTask(TaskData & task_data,
               QSharedPointer<HelperRegistry> const & helper_registry,
               QSharedPointer<StorageFrameworkClient> const & storage,
               QObject *parent = nullptr);
    virtual ~KeeperTask();

    Q_DISABLE_COPY(KeeperTask)

    bool start();
    QVariantMap state() const;
    void recalculate_task_state();

    static QVariantMap get_initial_state(KeeperTask::TaskData const &td);

    void cancel();

    QString to_string(Helper::State state);

    KeeperError error() const;
Q_SIGNALS:
    void task_state_changed(Helper::State state);
    void task_socket_ready(int socket_descriptor);

protected:
    KeeperTask(KeeperTaskPrivate & d, QObject *parent = nullptr);
    virtual QStringList get_helper_urls() const = 0;
    virtual void init_helper() = 0;

    QScopedPointer<KeeperTaskPrivate> const d_ptr;
};
