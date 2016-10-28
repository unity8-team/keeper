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
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#pragma once

#include "service/metadata-provider.h"
#include "util/connection-helper.h"

#include <QSharedPointer>
#include <QVector>

class StorageFrameworkClient;

/**
 * A MetadataProvider that lists the backups that can be restored
 */
class RestoreChoices: public MetadataProvider
{
public:
    explicit RestoreChoices(QObject *parent = nullptr);
    virtual ~RestoreChoices();
    QVector<Metadata> get_backups() const override;
    void get_backups_async() override;

private:
    QSharedPointer<StorageFrameworkClient> storage_;
    ConnectionHelper connections_;
    int manifests_to_read_ = 0;
};
