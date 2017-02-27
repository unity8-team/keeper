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
 */

#include <client/keeper-errors.h>

#include <QDebug>

QDBusArgument &operator<<(QDBusArgument &argument, keeper::Error value)
{
    argument.beginStructure();
    argument << static_cast<int>(value);
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, keeper::Error &val)
{
    int int_val;
    argument.beginStructure();
    argument >> int_val;
    val = static_cast<keeper::Error>(int_val);
    argument.endStructure();
    return argument;
}

namespace keeper
{
Error convert_from_dbus_variant(const QVariant & value, bool *conversion_ok)
{
    if (value.typeName() != QStringLiteral("QDBusArgument"))
    {
        qWarning() << Q_FUNC_INFO
                   << " Error converting dbus QVariant to Error, expected type is [ QDBusArgument ] and current type is: ["
                   << value.typeName() << "]";
        if (conversion_ok)
            *conversion_ok = false;
        return Error(keeper::Error::UNKNOWN);
    }
    auto dbus_arg = value.value<QDBusArgument>();

    if (dbus_arg.currentSignature() != "(i)")
    {
        qWarning() << Q_FUNC_INFO
                   << " Error converting dbus QVariant to Error, expected signature is \"(i)\" and current signature is: \""
                   << dbus_arg.currentSignature() << "\"";
        if (conversion_ok)
            *conversion_ok = false;
        return Error(keeper::Error::UNKNOWN);
    }
    Error ret;
    dbus_arg >> ret;

    if (conversion_ok)
        *conversion_ok = true;

    return ret;
}
}
