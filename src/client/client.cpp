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

#include <QTimer>

#include <client/client.h>

#include <qdbus-stubs/keeper_user_interface.h>

struct KeeperClientPrivate final
{
    Q_DISABLE_COPY(KeeperClientPrivate)

    KeeperClientPrivate(QObject* parent)
        : userIface(new DBusInterfaceKeeperUser(
                          DBusTypes::KEEPER_SERVICE,
                          DBusTypes::KEEPER_USER_PATH,
                          QDBusConnection::sessionBus()
                          )),
        status(""),
        progress(0),
        readyToBackup(false),
        backupBusy(false),
        stateTimer(parent)
    {
    }

    ~KeeperClientPrivate() = default;

    QScopedPointer<DBusInterfaceKeeperUser> userIface;
    QString status;
    QMap<QString, QVariantMap> backups;
    double progress;
    bool readyToBackup;
    bool backupBusy;
    QTimer stateTimer;
};

KeeperClient::KeeperClient(QObject* parent) :
    QObject(parent),
    d(new KeeperClientPrivate(this))
{
    DBusTypes::registerMetaTypes();

    // Store backups list locally with an additional "enabled" pair to keep track enabled states
    // TODO: We should be listening to a backupChoicesChanged signal to keep this list updated
    d->backups = getBackupChoices();
    for(auto iter = d->backups.begin(); iter != d->backups.end(); ++iter)
    {
        iter.value()["enabled"] = false;
    }

    connect(&d->stateTimer, &QTimer::timeout, this, &KeeperClient::stateUpdated);
}

KeeperClient::~KeeperClient() = default;

QStringList KeeperClient::backupUuids()
{
    QStringList returnList;
    for(auto iter = d->backups.begin(); iter != d->backups.end(); ++iter)
    {
        // TODO: We currently only support "folder" type backups
        if (iter.value().value("type").toString() == "folder")
        {
            returnList.append(iter.key());
        }
    }
    return returnList;
}

QString KeeperClient::status()
{
    return d->status;
}

double KeeperClient::progress()
{
    return d->progress;
}

bool KeeperClient::readyToBackup()
{
    return d->readyToBackup;
}

bool KeeperClient::backupBusy()
{
    return d->backupBusy;
}

void KeeperClient::enableBackup(QString uuid, bool enabled)
{
    d->backups[uuid]["enabled"] = enabled;

    for (auto const& backup : d->backups)
    {
        d->readyToBackup = false;

        if (backup.value("enabled") == true)
        {
            d->readyToBackup = true;
            break;
        }
    }

    Q_EMIT readyToBackupChanged();
}

void KeeperClient::startBackup()
{
    // TODO: Instead of polling for state, we should be listening to a stateChanged signal
    if (!d->stateTimer.isActive())
    {
        d->stateTimer.start(200);
    }

    // Determine which backups are enabled, and start only those
    QStringList backupList;
    for(auto iter = d->backups.begin(); iter != d->backups.end(); ++iter)
    {
        if (iter.value().value("enabled").toBool())
        {
            backupList.append(iter.key());
        }
    }

    if (!backupList.empty())
    {
        startBackup(backupList);

        d->status = "Preparing Backup...";
        Q_EMIT statusChanged();
        d->backupBusy = true;
        Q_EMIT backupBusyChanged();
    }
}

QString KeeperClient::getBackupName(QString uuid)
{
    return d->backups.value(uuid).value("display-name").toString();
}

QMap<QString, QVariantMap> KeeperClient::getBackupChoices() const
{
    QDBusReply<QMap<QString, QVariantMap>> choices = d->userIface->call("GetBackupChoices");

    if (!choices.isValid())
    {
        qWarning() << "Error getting backup choices:" << choices.error().message();
        return QMap<QString, QVariantMap>();
    }

    return choices.value();
}

void KeeperClient::startBackup(const QStringList& uuids) const
{
    QDBusReply<void> backupReply = d->userIface->call("StartBackup", uuids);

    if (!backupReply.isValid())
    {
        qWarning() << "Error starting backup:" << backupReply.error().message();
    }
}

QMap<QString, QVariantMap> KeeperClient::getState() const
{
    return d->userIface->state();
}

void KeeperClient::stateUpdated()
{
    auto states = getState();

    if (!states.empty())
    {
        // Calculate current total progress
        // TODO: May be better to monitor each backup's progress separately instead of total
        //       to avoid irregular jumps in progress between larger and smaller backups
        double totalProgress = 0;
        for (auto const& state : states)
        {
            totalProgress += state.value("percent-done").toDouble();
        }

        d->progress = totalProgress / states.count();
        Q_EMIT progressChanged();

        // Update backup status
        if (d->progress > 0 && d->progress < 1)
        {
            d->status = "Backup In Progress...";
            Q_EMIT statusChanged();

            d->backupBusy = true;
            Q_EMIT backupBusyChanged();
        }
        else if (d->progress >= 1)
        {
            d->status = "Backup Complete";
            Q_EMIT statusChanged();

            d->backupBusy = false;
            Q_EMIT backupBusyChanged();

            // Stop state timer
            if (d->stateTimer.isActive())
            {
                d->stateTimer.stop();
            }
        }
    }
}
