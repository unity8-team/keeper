/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Marcus Tomlinson <marcus.tomlinson@canonical.com>
 */

#pragma once

#include <QDBusPendingReply>

namespace utils
{

template <typename T>
T getOrThrow(QDBusPendingReply<T> const& pendingReply)
{
    const_cast<QDBusPendingReply<T>&>(pendingReply).waitForFinished();
    if (pendingReply.isError())
    {
        auto errorMessage = pendingReply.error().name() + ": " + pendingReply.error().message();
        throw std::runtime_error(errorMessage.toStdString());
    }
    return pendingReply;
}

}
