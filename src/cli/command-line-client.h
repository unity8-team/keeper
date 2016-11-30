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

class KeeperClient;
class CommandLineClientView;

class CommandLineClient : public QObject
{
    Q_OBJECT
public:
    explicit CommandLineClient(QObject * parent = nullptr);
    ~CommandLineClient();

    Q_DISABLE_COPY(CommandLineClient)

    void run_list_sections(bool remote);
    void run_backup(QStringList & sections);
    void run_restore(QStringList & sections);

public Q_SLOTS:
    void on_progress_changed();
    void on_status_changed();

private:
    void list_backup_sections(QMap<QString, QVariantMap> const & choices);
    void list_restore_sections(QMap<QString, QVariantMap> const & choices);
    QScopedPointer<KeeperClient> keeper_client_;
    QScopedPointer<CommandLineClientView> view_;
};
