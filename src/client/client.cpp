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
#include <qdbus-stubs/dbus-types.h>

struct KeeperClientPrivate final
{
    Q_DISABLE_COPY(KeeperClientPrivate)

    enum class TasksMode { IDLE_MODE, BACKUP_MODE, RESTORE_MODE };

    explicit KeeperClientPrivate(QObject* /* parent */)
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
    {
    }

    ~KeeperClientPrivate() = default;

    struct TaskStatus
    {
        QString status;
        double percentage;
    };

    static bool stateIsFinal(QString const & stateString)
    {
        return (stateString == "complete" || stateString == "cancelled" || stateString == "failed");
    }

    bool checkAllTasksFinished(keeper::KeeperItemsMap const & state) const
    {
        bool ret = true;
        for (auto iter = state.begin(); ret && (iter != state.end()); ++iter)
        {
            auto statusString = (*iter).value("action").toString();
            ret = stateIsFinal(statusString);
        }
        return ret;
    }

    static keeper::KeeperItemsMap getValue(QDBusMessage const & message, keeper::KeeperError & error)
    {
        if (message.errorMessage().isEmpty())
        {
            if (message.arguments().count() != 1)
            {
                error = keeper::KeeperError::ERROR_UNKNOWN;
                return keeper::KeeperItemsMap();
            }

            auto value = message.arguments().at(0);
            if (value.typeName() != QStringLiteral("QDBusArgument"))
            {
                error = keeper::KeeperError::ERROR_UNKNOWN;
                return keeper::KeeperItemsMap();
            }
            auto dbus_arg = value.value<QDBusArgument>();
            error = keeper::KeeperError::OK;
            keeper::KeeperItemsMap ret;
            dbus_arg >> ret;
            return ret;
        }
        if (message.arguments().count() != 2)
        {
            error = keeper::KeeperError::ERROR_UNKNOWN;
            return keeper::KeeperItemsMap();
        }

        // pick up the error
        bool valid;
        error = keeper::convert_from_dbus_variant(message.arguments().at(1), &valid);
        if (!valid)
        {
            error = keeper::KeeperError::ERROR_UNKNOWN;
        }
        return keeper::KeeperItemsMap();
    }

    QScopedPointer<DBusInterfaceKeeperUser> userIface;
    QScopedPointer<DBusPropertiesInterface> propertiesIface;
    QString status;
    keeper::KeeperItemsMap backups;
    double progress = 0;
    bool readyToBackup = false;
    bool backupBusy = false;
    QMap<QString, TaskStatus> taskStatus;
    TasksMode mode = TasksMode::IDLE_MODE;
};

KeeperClient::KeeperClient(QObject* parent) :
    QObject(parent),
    d(new KeeperClientPrivate(this))
{
    DBusTypes::registerMetaTypes();

    // Store backups list locally with an additional "enabled" pair to keep track enabled states
    // TODO: We should be listening to a backupChoicesChanged signal to keep this list updated
    keeper::KeeperError error;
    d->backups = getBackupChoices(error);

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

    d->taskStatus[uuid] = KeeperClientPrivate::TaskStatus{"", 0.0};

    Q_EMIT readyToBackupChanged();
}

void KeeperClient::enableRestore(QString uuid, bool enabled)
{
    // Until we re-design the client we treat restores as backups
    enableBackup(uuid, enabled);
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

        d->mode = KeeperClientPrivate::TasksMode::BACKUP_MODE;
        d->status = "Preparing Backup...";
        Q_EMIT statusChanged();
        d->backupBusy = true;
        Q_EMIT backupBusyChanged();
    }
}

void KeeperClient::startRestore()
{
    // Determine which restores are enabled, and start only those
    QStringList restoreList;
    for(auto iter = d->backups.begin(); iter != d->backups.end(); ++iter)
    {
        if (iter.value().value("enabled").toBool())
        {
            restoreList.append(iter.key());
        }
    }

    if (!restoreList.empty())
    {
        startRestore(restoreList);

        d->mode = KeeperClientPrivate::TasksMode::RESTORE_MODE;
        d->status = "Preparing Restore...";
        Q_EMIT statusChanged();
        d->backupBusy = true;
        Q_EMIT backupBusyChanged();
    }
}

QString KeeperClient::getBackupName(QString uuid)
{
    return d->backups.value(uuid).value("display-name").toString();
}

keeper::KeeperItemsMap KeeperClient::getBackupChoices(keeper::KeeperError & error) const
{
    QDBusMessage choices = d->userIface->call("GetBackupChoices");
    return KeeperClientPrivate::getValue(choices, error);
}

keeper::KeeperItemsMap KeeperClient::getRestoreChoices(keeper::KeeperError & error) const
{
    QDBusMessage choices = d->userIface->call("GetRestoreChoices");
    return KeeperClientPrivate::getValue(choices, error);
}

void KeeperClient::startBackup(const QStringList& uuids) const
{
    QDBusReply<void> backupReply = d->userIface->call("StartBackup", uuids);

    if (!backupReply.isValid())
    {
        qWarning() << "Error starting backup:" << backupReply.error().message();
    }
}

void KeeperClient::startRestore(const QStringList& uuids) const
{
    QDBusReply<void> backupReply = d->userIface->call("StartRestore", uuids);

    if (!backupReply.isValid())
    {
        qWarning() << "Error starting restore:" << backupReply.error().message();
    }
}

keeper::KeeperItemsMap KeeperClient::getState() const
{
    return d->userIface->state();
}

void KeeperClient::stateUpdated()
{
    auto states = getState();

    if (!states.empty())
    {
        for (auto const & uuid : d->taskStatus.keys())
        {
            auto iter_state = states.find(uuid);
            if (iter_state == states.end())
            {
                qWarning() << "State for uuid: " << uuid << " was not found";
            }
            keeper::KeeperItem keeper_item((*iter_state));
            auto progress = keeper_item.get_percent_done();
            auto status = keeper_item.get_status();
            auto keeper_error = keeper_item.get_error();

            auto current_state = d->taskStatus[uuid];
            if (current_state.status != status || current_state.percentage < progress)
            {
                d->taskStatus[uuid].status = status;
                d->taskStatus[uuid].percentage = progress;
                Q_EMIT(taskStatusChanged(keeper_item.get_display_name(), status, progress, keeper_error));
            }
        }

        double totalProgress = 0;
        for (auto const& state : states)
        {
            keeper::KeeperItem keeper_item(state);
            totalProgress += keeper_item.get_percent_done();
        }

        d->progress = totalProgress / states.count();
        Q_EMIT progressChanged();

        auto allTasksFinished = d->checkAllTasksFinished(states);
        // Update backup status
        QString statusString;
        if (d->mode == KeeperClientPrivate::TasksMode::BACKUP_MODE)
        {
            statusString = QStringLiteral("Backup");
        }
        else if (d->mode == KeeperClientPrivate::TasksMode::RESTORE_MODE)
        {
            statusString = QStringLiteral("Restore");
        }
        if (d->progress > 0 && d->progress < 1)
        {
            d->status = statusString + QStringLiteral(" In Progress...");
            Q_EMIT statusChanged();

            d->backupBusy = true;
            Q_EMIT backupBusyChanged();
        }
        else if (d->progress >= 1 && !allTasksFinished)
        {
            d->status = statusString + QStringLiteral(" Finishing...");
            Q_EMIT statusChanged();

            d->backupBusy = true;
            Q_EMIT backupBusyChanged();
        }
        else if (allTasksFinished)
        {
            d->status = statusString + QStringLiteral(" Complete");
            Q_EMIT statusChanged();

            d->backupBusy = false;
            Q_EMIT backupBusyChanged();
        }

        if (d->checkAllTasksFinished(states))
        {
            Q_EMIT(finished());
        }
    }
}
