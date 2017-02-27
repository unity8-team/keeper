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
 *     Xavi Garcia <xavi.garcia.mena@canonical.com>
 */
#pragma once

#include <QMap>
#include <QObject>
#include <QScopedPointer>
#include <QTimer>
#include "../../include/client/keeper-items.h"

class KeeperClient;
class CommandLineClientView;

class CommandLineClient : public QObject
{
    Q_OBJECT
public:
    explicit CommandLineClient(QObject * parent = nullptr);
    ~CommandLineClient();

    Q_DISABLE_COPY(CommandLineClient)

    void run_list_sections(bool remote, QString const & storage = "");
    void run_list_storage_accounts();
    void run_backup(QStringList & sections, QString const & storage);
    void run_restore(QStringList & sections, QString const & storage);
    void run_cancel() const;

private Q_SLOTS:
    void on_progress_changed();
    void on_status_changed();
    void on_keeper_client_finished();

private:
    bool find_choice_value(QVariantMap const & choice, QString const & id, QVariant & value);
    void list_backup_sections(keeper::Items const & choices);
    void list_restore_sections(keeper::Items const & choices);
    void list_storage_accounts(QStringList const & accounts);
    void check_for_choices_error(keeper::Error error);
    QScopedPointer<KeeperClient> keeper_client_;
    QScopedPointer<CommandLineClientView> view_;
};
