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
 *   Charles Kerr <charles.kerr@canoincal.com>
 */

#pragma once

#include <QStringList>

#include <cstddef> // ssize_t
#include <memory> // shared_ptr
#include <vector>


class TarCreator
{
public:
    TarCreator(const QStringList& files, bool compress);
    ~TarCreator();

    ssize_t calculate_size() const;
    bool step(std::vector<char>& fillme);

private:
    class Impl;
    friend class Impl;
    std::shared_ptr<Impl> impl_;
};
