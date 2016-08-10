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

#include <client/client.h>

#include <qdbus-stubs/keeper_user_interface.h>

class KeeperClientPrivate final
{
    Q_DISABLE_COPY(KeeperClientPrivate)

public:
    KeeperClientPrivate()
        : user_iface(new DBusInterfaceKeeperUser(
                         DBusTypes::KEEPER_SERVICE,
                         DBusTypes::KEEPER_USER_PATH,
                         QDBusConnection::sessionBus()
                         ))
    {
    }

    ~KeeperClientPrivate() = default;

    QScopedPointer<DBusInterfaceKeeperUser> user_iface;
};

KeeperClient::KeeperClient(QObject* parent)
    : QObject{parent}
    , d_ptr(new KeeperClientPrivate())
{
    DBusTypes::registerMetaTypes();
}

KeeperClient::~KeeperClient() = default;

QMap<QString, QVariantMap> KeeperClient::GetBackupChoices() const
{
    QDBusReply<QMap<QString, QVariantMap>> choices = d_ptr->user_iface->call("GetBackupChoices");

    if (!choices.isValid())
    {
        qWarning() << "Error getting backup choices: " << choices.error().message();
        return QMap<QString, QVariantMap>();
    }

    return choices.value();
}

void KeeperClient::StartBackup(const QStringList& uuids) const
{
    QDBusReply<void> backup_reply = d_ptr->user_iface->call("StartBackup", uuids);

    if (!backup_reply.isValid())
    {
        qWarning() << "Error starting backup: " << backup_reply.error().message();
    }
}

QMap<QString, QVariantMap> KeeperClient::GetState() const
{
    return d_ptr->user_iface->state();
}
