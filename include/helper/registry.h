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

#include "helper/metadata.h"

#include <QString>

class HelperRegistry
{
public:
    virtual ~HelperRegistry() =default;
    Q_DISABLE_COPY(HelperRegistry)

    virtual QStringList get_backup_helper_urls(Metadata const& task) =0;
    virtual QStringList get_restore_helper_urls(Metadata const& task) =0;

protected:
    HelperRegistry() =default;
};
