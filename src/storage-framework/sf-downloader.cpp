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
 *   Xavi Garcia Mena <xavi.garcia.mena@canonical.com>
 */

#include "storage-framework/sf-downloader.h"

#include <QFuture>
#include <QFutureWatcher>

StorageFrameworkDownloader::StorageFrameworkDownloader(
    unity::storage::qt::client::Downloader::SPtr const& downloader,
    qint64 file_size,
    QObject *parent
):
    Downloader(parent),
    downloader_(downloader),
    file_size_(file_size)
{
    qDebug() << "StorageFrameworkDownloader";
}

std::shared_ptr<QLocalSocket>
StorageFrameworkDownloader::socket()
{
    return downloader_->socket();
}

void
StorageFrameworkDownloader::finish()
{
    qDebug() << Q_FUNC_INFO << "is finishing";

//    connections_.connect_future(
//        uploader_->finish_upload(),
//        std::function<void(std::shared_ptr<unity::storage::qt::client::File> const&)>{
//            [this](std::shared_ptr<unity::storage::qt::client::File> const& file){
//                auto const success = bool(file);
//                qDebug() << "commit finished with" << success;
//                if (success)
//                {
//                    file_name_after_commit_ = file->name();
//                }
//                Q_EMIT(commit_finished(success));
//            }
//        }
//    );
    downloader_->finish_download();
    Q_EMIT(download_finished()); // TODO add the code to call finish_download
}

qint64
StorageFrameworkDownloader::file_size() const
{
    return file_size_;
}
