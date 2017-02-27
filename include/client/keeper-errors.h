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
 *     Xavi Garcia Mena <xavi.garcia.mena@canonical.com>
 */

#pragma once

#include <QDBusArgument>
#include <QMetaType>

namespace keeper
{

enum class Error
{
    OK,
    UNKNOWN,
    HELPER_READ,
    HELPER_WRITE,
    HELPER_INACTIVITY_DETECTED,
    HELPER_SOCKET,
    HELPER_START_TIMEOUT,
    NO_HELPER_INFORMATION_IN_REGISTRY,
    HELPER_BAD_URL,
    MANIFEST_STORAGE,
    COMMITTING_DATA,

    CREATING_REMOTE_DIR,
    CREATING_REMOTE_FILE,
    READING_REMOTE_FILE,
    REMOTE_DIR_NOT_EXISTS,
    NO_REMOTE_ACCOUNTS,
    NO_REMOTE_ROOTS,
    ACCOUNT_NOT_FOUND
};

Error convert_from_dbus_variant(const QVariant & value, bool *conversion_ok = nullptr);
} // namespace keeper

QDBusArgument &operator<<(QDBusArgument &argument, keeper::Error value);
const QDBusArgument &operator>>(const QDBusArgument &argument, keeper::Error &val);

Q_DECLARE_METATYPE(keeper::Error)
