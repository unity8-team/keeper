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

#include "storage/storage-factory.h"

#include <QtDebug>

namespace storage
{

class StorageFactory::Impl
{
public:

    Impl()
    {
        const auto modeStr = QString::fromUtf8(qgetenv("KEEPER_BACKEND"));

        if (modeStr.isEmpty())
            mode_ = DEFAULT_MODE;
        else if (modeStr == "storage-framework")
            mode_ = Mode::StorageFramework;
        else if (modeStr == "memory")
            mode_ = Mode::Memory;
        else if (modeStr == "local")
            mode_ = Mode::Local;
        else {
            const auto modeStr8 = modeStr.toUtf8();
            qFatal("Unrecognized keeper backend: %s", modeStr8.constData());
        }
    }

    QFuture<Storage> getStorage()
    {
        return QFuture<Storage>(); // FIXME
    }

private:

    enum class Mode { StorageFramework, Local, Memory };
    static constexpr Mode DEFAULT_MODE {Mode::Local}; // while under development
    Mode mode_ {Mode::Local};
};

/***
****
***/

StorageFactory::StorageFactory():
    impl_{new Impl{}}
{
}

StorageFactory::~StorageFactory()
{
}

QFuture<Storage>
StorageFactory::getStorage()
{
    return impl_->getStorage();
}


} // namespace storage
