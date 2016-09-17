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

#include "storage-framework/uploader.h"
#include "helper/helper.h" // parent class
#include "helper/registry.h"

#include <QObject>
#include <QScopedPointer>
#include <QString>

#include <memory>

class BackupHelperPrivate;
class BackupHelper final: public Helper
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

    static constexpr int MAX_INACTIVITY_TIME = 15000;

    void set_uploader(std::shared_ptr<Uploader> const& uploader);
    void start(QStringList const& urls) override;
    void stop() override;
    int get_helper_socket() const;
    QString to_string(Helper::State state) const override;
    void set_state(State) override;
protected:
    void on_helper_finished() override;

private:
    QScopedPointer<BackupHelperPrivate> const d_ptr;
};
