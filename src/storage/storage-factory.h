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
 *     Charles Kerr <charles.kerr@canonical.com>
 */

#pragma once

#include "storage/storage.h"

#include <QFuture>
#include <QSharedPointer>

namespace storage
{

class StorageFactory: public QObject
{
public:

    StorageFactory();
    ~StorageFactory();

    StorageFactory(StorageFactory const&) =delete;
    StorageFactory& operator=(StorageFactory const&) =delete;
    StorageFactory(StorageFactory&&) =delete;
    StorageFactory& operator=(StorageFactory&&) =delete;

    QFuture<QSharedPointer<Storage>> getStorage();

private:

    class Impl;
    friend class Impl;
    QSharedPointer<Impl> impl_;
};

}
