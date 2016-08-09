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

#include "helper/registry.h" // parent class

#include <QObject>
#include <QString>

#include <memory>

/**
 * A simple hardcoded registry of Keeper 1.0's builtin helpers.
 *
 * TODO: allow 3rd party apps to supply their own helpers
 */
class StaticRegistry: public HelperRegistry
{
public:
    StaticRegistry();
    ~StaticRegistry();

    Q_DISABLE_COPY(StaticRegistry)

    QStringList get_backup_helper_urls(Metadata const& metadata) override;

private:
    class Impl;
    friend class Impl;
    std::unique_ptr<Impl> impl_;
};
