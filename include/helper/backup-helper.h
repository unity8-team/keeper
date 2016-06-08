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

#include <QObject>

namespace internal
{
class BackupHelperImpl;
}

class BackupHelper : public QObject
{
    Q_OBJECT
public:
    Q_DISABLE_COPY(BackupHelper)

    BackupHelper(QString const &appid, QObject * parent=nullptr);
    virtual ~BackupHelper();

    void start(int socket);
    void stop();

Q_SIGNALS:
    void started();
    void finished();
    void stalled();

private:
    QScopedPointer<internal::BackupHelperImpl> p_;
};

