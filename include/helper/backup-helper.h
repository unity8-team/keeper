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
 *     Charles Kerr <charles.kerr@canonical.com>
 */

#pragma once

#include <helper/helper.h> // parent class
#include <helper/registry.h>

#include <QObject>
#include <QScopedPointer>
#include <QLocalSocket>
#include <QString>

#include <memory>

class BackupHelperPrivate;
class BackupHelper : public Helper
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(BackupHelper)

public:
    BackupHelper(
        QString const & appid,
        clock_func const & clock=Helper::default_clock,
        QObject * parent=nullptr
    );
    virtual ~BackupHelper();
    Q_DISABLE_COPY(BackupHelper)

    static constexpr int MAX_INACTIVITY_TIME = 10000;

    void set_storage_framework_socket(std::shared_ptr<QLocalSocket> const& sf_socket);
    void start(QStringList const& urls) override;
    void stop() override;
    void on_helper_process_stopped() override;
    int get_helper_socket() const;

private:
    QScopedPointer<BackupHelperPrivate> const d_ptr;
};
