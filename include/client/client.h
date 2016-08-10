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

#define KEEPER_EXPORT __attribute__((visibility("default")))

#include <QObject>
#include <QScopedPointer>

class KeeperClientPrivate;

class KEEPER_EXPORT KeeperClient final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KeeperClient)

public:
    explicit KeeperClient(QObject* parent = nullptr);
    ~KeeperClient();

    QMap<QString, QVariantMap> GetBackupChoices() const;
    void StartBackup(QStringList const& uuids) const;

    QMap<QString, QVariantMap> GetState() const;

Q_SIGNALS:

private:
    QScopedPointer<KeeperClientPrivate> const d_ptr;
};
