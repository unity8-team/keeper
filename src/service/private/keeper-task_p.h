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
#include "../keeper-task.h"

class KeeperTaskPrivate
{
     Q_DECLARE_PUBLIC(KeeperTask)
public:
    KeeperTaskPrivate(KeeperTask * keeper_task,
                      KeeperTask::TaskData & task_data,
                      QSharedPointer<HelperRegistry> const & helper_registry,
                      QSharedPointer<StorageFrameworkClient> const & storage);

    virtual ~KeeperTaskPrivate();

    bool start();
    QVariantMap state() const;
    void ask_for_storage_framework_socket(quint64 n_bytes);

    void cancel();

    static QVariantMap get_initial_state(KeeperTask::TaskData const &td);

    QString to_string(Helper::State state);
protected:
    void set_current_task_action(QString const& action);
    void on_helper_percent_done_changed(float percent_done);
    QVariantMap calculate_task_state();
    void calculate_and_notify_state(Helper::State state);
    void recalculate_task_state();
    void on_backup_socket_ready(std::shared_ptr<QLocalSocket> const &  sf_socket);

    virtual void on_helper_state_changed(Helper::State state);

    KeeperTask * const q_ptr;
    KeeperTask::TaskData & task_data_;
    QSharedPointer<HelperRegistry> helper_registry_;
    QSharedPointer<StorageFrameworkClient> storage_;
    QSharedPointer<Helper> helper_;
    QVariantMap state_;
};
