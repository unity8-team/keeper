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
 *   Xavi Garcia Mena <xavi.garcia.mena@canonical.com>
 */

#include "manifest.h"

#include "storage-framework/storage_framework_client.h"
#include "util/connection-helper.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSharedPointer>
#include <QVector>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace sf = unity::storage::qt::client;

// JSON Keys
namespace
{
    constexpr const char ENTRIES_KEY[] = "entries";
    constexpr const char MANIFEST_FILE_NAME[] = "manifest.json";
}

/***
****
***/

class ManifestPrivate
{
public:
    ManifestPrivate(QSharedPointer<StorageFrameworkClient> const & storage, QString const & dir, Manifest * manifest)
        : q_ptr{manifest}
        , storage_{storage}
        , dir_{dir}
    {
    }

    ~ManifestPrivate() = default;

    Q_DISABLE_COPY(ManifestPrivate)

    void add_entry(Metadata const & entry)
    {
        entries_.push_back(entry);
    }

    void store()
    {
        qDebug() << "Metadata asking storage framework for a socket";
        auto json_data = to_json();
        auto n_bytes = json_data.size();

        connections_.connect_future(
            storage_->get_new_uploader(n_bytes, dir_, MANIFEST_FILE_NAME),
            std::function<void(std::shared_ptr<Uploader> const&)>{
                [this, json_data](std::shared_ptr<Uploader> const& uploader){
                    qDebug() << "Manifest uploader is" << static_cast<void*>(uploader.get());
                    if (uploader)
                    {
                        auto socket = uploader->socket();
                        socket->write(json_data);
                        connections_.connect_oneshot(
                            uploader.get(),
                            &Uploader::commit_finished,
                            std::function<void(bool)>{[this, uploader](bool success){
                                qDebug() << "Metadata commit finished";
                                if (!success)
                                {
                                    finish_with_error(QStringLiteral("Error committing manifest file to storage-framework"));
                                }
                                else
                                {
                                    uploader_committed_file_name_ = uploader->file_name();
                                    finish();
                                }
                            }}
                        );
                        uploader->commit();
                    }
                    else
                    {
                        finish_with_error(QStringLiteral("Error retrieving uploader for manifest file from storage-framework"));
                    }
                }
            }
        );
    }

    void read()
    {
        connections_.connect_future(
            storage_->get_new_downloader(dir_, MANIFEST_FILE_NAME),
            std::function<void(std::shared_ptr<Downloader> const&)>{
                [this](std::shared_ptr<Downloader> const& downloader){
                    qDebug() << "Manifest downloader is" << static_cast<void*>(downloader.get());
                    if (downloader)
                    {
                        auto socket = downloader->socket();
                        if (socket->atEnd())
                        {
                            if (!socket->waitForReadyRead(5000))
                            {
                                qWarning() << "Manifest socket was not ready to read after timeout";
                            }
                        }
                        auto json_content = socket->readAll();
                        from_json(json_content);
                        downloader->finish();
                        finish();
                    }
                    else
                    {
                        finish_with_error(QStringLiteral("Error retrieving downloader for manifest file from storage-framework"));
                    }
                }
            }
        );
    }

    QVector<Metadata> get_entries()
    {
        return entries_;
    }

    QString error() const
    {
        return error_string_;
    }

    QByteArray to_json() const
    {
        QJsonArray json_array;
        for (auto metadata : entries_)
        {
            json_array.append(metadata.json());
        }
        QJsonObject json_root;
        json_root[ENTRIES_KEY] = json_array;
        QJsonDocument doc(json_root);

        return doc.toJson(QJsonDocument::Compact);
    }

    void from_json(QByteArray const & json)
    {
        auto doc_read = QJsonDocument::fromJson(json);

        auto json_read_root = doc_read.object();
        auto items = json_read_root[ENTRIES_KEY].toArray();

        QVector<Metadata> read_metadata;
        for( auto iter = items.begin(); iter != items.end(); ++iter)
        {
            entries_.push_back(Metadata((*iter).toObject()));
        }
    }

private:

    void finish_with_error(QString const & message)
    {
        error_string_ = message;
        Q_EMIT(q_ptr->finished(false));
    }

    void finish()
    {
        error_string_ = "";
        Q_EMIT(q_ptr->finished(true));
    }

    Manifest * const q_ptr;
    QSharedPointer<StorageFrameworkClient> storage_;
    QString dir_;

    QVector<Metadata> entries_;
    QString error_string_;
    QString uploader_committed_file_name_;

    ConnectionHelper connections_;
};

/***
****
***/

Manifest::Manifest(QSharedPointer<StorageFrameworkClient> const & storage, QString const & dir, QObject * parent)
    : QObject (parent)
    , d_ptr{new ManifestPrivate{storage, dir, this}}
{
}

Manifest::~Manifest() = default;


void Manifest::add_entry(Metadata const & entry)
{
    Q_D(Manifest);

    d->add_entry(entry);
}

void Manifest::store()
{
    Q_D(Manifest);

    d->store();
}

void Manifest::read()
{
    Q_D(Manifest);

    d->read();
}

QVector<Metadata> Manifest::get_entries()
{
    Q_D(Manifest);

    return d->get_entries();
}

QString Manifest::error() const
{
    Q_D(const Manifest);

    return d->error();
}
