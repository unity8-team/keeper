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

#include "service/restore-choices.h"

#include "service/manifest.h"
#include "storage-framework/storage_framework_client.h"

#include <QDebug>

using namespace unity::storage::qt::client;


RestoreChoices::RestoreChoices(QObject *parent)
    : MetadataProvider(parent)
    , storage_(new StorageFrameworkClient)
{
}

RestoreChoices::~RestoreChoices() = default;

QVector<Metadata>
RestoreChoices::get_backups() const
{
    return backups_;
}

void
RestoreChoices::get_backups_async()
{
    backups_.clear();
    connections_.connect_future(
        storage_->get_keeper_dirs(),
        std::function<void(QVector<QString> const &)>{
            [this](QVector<QString> const & dirs){
                if (dirs.size() > 0)
                {
                    manifests_to_read_ = dirs.size();
                    for (auto i = 0; i < dirs.size(); ++i)
                    {
                        QSharedPointer<Manifest> manifest(new Manifest(storage_, dirs.at(i)), [](Manifest *m){m->deleteLater();});
                        connections_.connect_oneshot(
                            manifest.data(),
                            &Manifest::finished,
                            std::function<void(bool)>{[this, dirs, manifest, i](bool success){
                                qDebug() << "Finished reading manifest in dir: " << dirs.at(i) << " success =" << success;
                                if (success)
                                {
                                    this->backups_ += manifest->get_entries();
                                }
                                manifests_to_read_--;
                                if (!manifests_to_read_)
                                {
                                     Q_EMIT(finished(keeper::KeeperError::OK));
                                }
                            }}
                        );
                        manifest->read();
                    }
                }
                else
                {
                    qWarning() << "We could not find and keeper backups directory when retrieving restore options.";
                    Q_EMIT(finished(storage_->get_last_error()));
                }
            }
        }
    );
}
