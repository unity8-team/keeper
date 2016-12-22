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
 *     Marcus Tomlinson <marcus.tomlinson@canonical.com>
 */

#pragma once

#include "keeper-errors.h"

#include <QObject>
#include <QScopedPointer>
#include <QStringList>
#include <QVariant>
#include "keeper-items.h"

struct KeeperClientPrivate;

class Q_DECL_EXPORT KeeperClient : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KeeperClient)

// QML
public:
    explicit KeeperClient(QObject* parent = nullptr);
    ~KeeperClient();

    Q_PROPERTY(QStringList backupUuids READ backupUuids CONSTANT)
    QStringList backupUuids();

    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    QString status();

    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    double progress();

    Q_PROPERTY(bool readyToBackup READ readyToBackup NOTIFY readyToBackupChanged)
    bool readyToBackup();

    Q_PROPERTY(bool backupBusy READ backupBusy NOTIFY backupBusyChanged)
    bool backupBusy();

    Q_INVOKABLE QString getBackupName(QString uuid);
    Q_INVOKABLE void enableBackup(QString uuid, bool enabled);
    Q_INVOKABLE void startBackup();

    Q_INVOKABLE void enableRestore(QString uuid, bool enabled);
    Q_INVOKABLE void startRestore();

// C++
public:
    keeper::Items getBackupChoices(keeper::Error & error) const;
    keeper::Items getRestoreChoices(keeper::Error & error) const;
    void startBackup(QStringList const& uuids) const;
    void startRestore(QStringList const& uuids) const;

    keeper::Items getState() const;

Q_SIGNALS:
    void statusChanged();
    void progressChanged();
    void readyToBackupChanged();
    void backupBusyChanged();

    void taskStatusChanged(QString const & displayName, QString const & status, double percentage, keeper::Error error);
    void finished();

private Q_SLOTS:
    void stateUpdated();

private:
    QScopedPointer<KeeperClientPrivate> const d;
};
