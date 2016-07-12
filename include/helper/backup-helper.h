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

#include <QObject>
#include <QScopedPointer>

class BackupHelperPrivate;
class BackupHelper : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(BackupHelper)

public:
    BackupHelper(QString const &appid, QObject * parent=nullptr);
    virtual ~BackupHelper();
    Q_DISABLE_COPY(BackupHelper)

    void start(int sd);
    void stop();
    int get_helper_socket() const;

    void emitHelperStarted();
    void emitHelperFinished();

Q_SIGNALS:
    void started();
    void finished();
    void stalled();

private:
    QScopedPointer<BackupHelperPrivate> const d_ptr;
};
