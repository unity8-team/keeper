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

#include <QDBusContext>
#include <QDBusUnixFileDescriptor>
#include <QObject>

class Keeper;
class KeeperHelper : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    explicit KeeperHelper(Keeper *parent);
    virtual ~KeeperHelper();
    Q_DISABLE_COPY(KeeperHelper)

public Q_SLOTS:
    QDBusUnixFileDescriptor StartBackup(quint64 nbytes);
    QDBusUnixFileDescriptor StartRestore();

    void UpdateStatus(const QString &app_id, const QString &status, double percentage);

private:
    Keeper& keeper_;
};
