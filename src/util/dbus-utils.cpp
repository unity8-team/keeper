/*
 * Copyright (C) 2015 Canonical, Ltd.
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
 * Author: Pete Woods <pete.woods@canonical.com>
 */

#include <util/dbus-utils.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>
#include <memory>

namespace DBusUtils
{

namespace
{
typedef QPair<QString, QString> QStringPair;
typedef QPair<QDBusConnection, QVariantMap> ConnectionValues;
typedef QMap<QStringPair, ConnectionValues> PropertyChangeQueue;

static std::shared_ptr<QTimer> propertyChangeTimer;

static PropertyChangeQueue propertyChangeQueue;
}

void flushPropertyChanges()
{
    if (!propertyChangeTimer)
    {
        return;
    }

    if (propertyChangeTimer->isActive())
    {
        propertyChangeTimer->stop();
    }

    QMapIterator<QStringPair, ConnectionValues> it(propertyChangeQueue);
    while (it.hasNext())
    {
        it.next();

        QDBusMessage signal = QDBusMessage::createSignal(
              it.key().first, // Path
              "org.freedesktop.DBus.Properties",
              "PropertiesChanged");

        // Interface
        signal << it.key().second;
        // Changed properties (name, value)
        signal << it.value().second;
        signal << QStringList();
        // Connection
        it.value().first.send(signal);
    }

    propertyChangeQueue.clear();
}

void notifyPropertyChanged(
        const QDBusConnection& connection,
        const QObject& o,
        const QString& path,
        const QString& interface,
        const QStringList& propertyNames)
{
    if (!propertyChangeTimer)
    {
        propertyChangeTimer = std::make_shared<QTimer>();
        propertyChangeTimer->setInterval(0);
        propertyChangeTimer->setSingleShot(true);
        propertyChangeTimer->setTimerType(Qt::CoarseTimer);

        QObject::connect(propertyChangeTimer.get(), &QTimer::timeout, &flushPropertyChanges);
    }

    QStringPair key(path, interface);
    auto it = propertyChangeQueue.find(key);
    if (it == propertyChangeQueue.end())
    {
        it = propertyChangeQueue.insert(key, ConnectionValues(connection, QVariantMap()));
    }

    for (const auto& propertyName: propertyNames)
    {
        it->second.insert(propertyName, o.property(qPrintable(propertyName)));
    }

    propertyChangeTimer->start();
}

}
