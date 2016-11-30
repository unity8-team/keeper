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
#include <qdbus-stubs/DBusPropertiesInterface.h>

struct KeeperClientPrivate final
{
    Q_DISABLE_COPY(KeeperClientPrivate)

    KeeperClientPrivate(QObject* /* parent */)
        : userIface(new DBusInterfaceKeeperUser(
                          DBusTypes::KEEPER_SERVICE,
                          DBusTypes::KEEPER_USER_PATH,
                          QDBusConnection::sessionBus()
                          ))
        , propertiesIface(new DBusPropertiesInterface(
                           DBusTypes::KEEPER_SERVICE,
                           DBusTypes::KEEPER_USER_PATH,
                           QDBusConnection::sessionBus()
                         ))
        , status("")
        , progress(0)
        , readyToBackup(false)
        , backupBusy(false)
    {
    }

    ~KeeperClientPrivate() = default;

    struct TaskStatus
    {
        QString status;
        double percentage;
    };

    QScopedPointer<DBusInterfaceKeeperUser> userIface;
    QScopedPointer<DBusPropertiesInterface> propertiesIface;
    QString status;
    QMap<QString, QVariantMap> backups;
    double progress;
    bool readyToBackup;
    bool backupBusy;
    QMap<QString, TaskStatus> task_status;
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

    connect(d->propertiesIface.data(), &DBusPropertiesInterface::PropertiesChanged, this, &KeeperClient::stateUpdated);
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

    d->task_status[uuid] = KeeperClientPrivate::TaskStatus{"", 0.0};

    Q_EMIT readyToBackupChanged();
}

void KeeperClient::startBackup()
{
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

QMap<QString, QVariantMap> KeeperClient::getRestoreChoices() const
{
    QDBusPendingReply<QMap<QString, QVariantMap>> choices = d->userIface->call("GetRestoreChoices");

    choices.waitForFinished();
    if (!choices.isValid())
    {
        qWarning() << "Error getting restore choices:" << choices.error().message();
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
        for (auto const & uuid : d->task_status.keys())
        {
            auto iter_state = states.find(uuid);
            if (iter_state == states.end())
            {
                qWarning() << "State for uuid: " << uuid << " was not found";
            }
            auto state = (*iter_state);
            double progress = state.value("percent-done").toDouble();
            auto status = state.value("action").toString();

            auto current_state = d->task_status[uuid];
            if (current_state.status != status || current_state.percentage < progress)
            {
                d->task_status[uuid].status = status;
                d->task_status[uuid].percentage = progress;
                Q_EMIT(taskStatusChanged(state.value("display-name").toString(), status, progress));
            }
        }

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
        }
    }
}
