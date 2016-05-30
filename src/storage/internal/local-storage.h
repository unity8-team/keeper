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

#include <storage/storage.h>

#include <memory> // unique_ptr

namespace storage
{
namespace internal
{

// TODO: use the Qt pimpl macros
class LocalStorage final: public Storage
{
public:

    LocalStorage();
    virtual ~LocalStorage();

    LocalStorage(LocalStorage&&) =delete;
    LocalStorage(LocalStorage const&) =delete;
    LocalStorage& operator=(LocalStorage&&) =delete;
    LocalStorage& operator=(LocalStorage const&) =delete;

    QVector<BackupInfo> getBackups() override;
    QSharedPointer<QIODevice> startRestore(const QString& uuid) override;
    QSharedPointer<QIODevice> startBackup(const BackupInfo& info) override;

private:

    class Impl;
    friend class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace internal

} // namespace storage
