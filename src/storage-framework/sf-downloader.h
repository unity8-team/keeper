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

#include "util/connection-helper.h"
#include "storage-framework/downloader.h"

#include <unity/storage/qt/client/client-api.h>

#include <QLocalSocket>

#include <memory>

class StorageFrameworkDownloader final: public Downloader
{
public:

    StorageFrameworkDownloader(unity::storage::qt::client::Downloader::SPtr const& uploader,
                               qint64 file_size,
                               QObject * parent = nullptr);
    std::shared_ptr<QLocalSocket> socket() override;
    void finish() override;
    qint64 file_size() const override;

private:

    unity::storage::qt::client::Downloader::SPtr const downloader_;
    qint64 file_size_;

    ConnectionHelper connections_;
};
