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

#include "service/metadata-provider.h"

#include <unity/storage/qt/client/client-api.h>

#include <QVector>

/**
 * A MetadataProvider that lists the backups that can be restored
 */
class RestoreChoices: public MetadataProvider
{
public:
    RestoreChoices();
    virtual ~RestoreChoices();
    QVector<Metadata> get_backups() override;

private:
    unity::storage::qt::client::Runtime::SPtr runtime_;
};
