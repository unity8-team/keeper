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

class KeeperClientPrivate final
{
    Q_DISABLE_COPY(KeeperClientPrivate)

public:
    KeeperClientPrivate() = default;
    ~KeeperClientPrivate() = default;
};

KeeperClient::KeeperClient(QObject* parent)
    : QObject{parent}
    , d_ptr(new KeeperClientPrivate())
{
}

KeeperClient::~KeeperClient() = default;
