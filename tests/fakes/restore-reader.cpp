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
 *     Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#include "restore-reader.h"

#include <QCoreApplication>
#include <QCryptographicHash>

#include <unistd.h>

//static constexpr int UPLOAD_BUFFER_MAX_ {1024*16};
constexpr int UPLOAD_BUFFER_MAX_ = 16 * 1024;

RestoreReader::RestoreReader(qint64 fd, QString const & file_path, QObject * parent)
    : QObject(parent)
    , file_(file_path)
{
    socket_.setSocketDescriptor(fd);
    connect(&socket_, &QLocalSocket::readyRead, this, &RestoreReader::read_all);
    connect(&socket_, &QLocalSocket::disconnected, this, &RestoreReader::finish);
    if (!file_.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qFatal("Error opening file");
    }

    read_all();
}

void RestoreReader::read_all()
{
    qDebug() << Q_FUNC_INFO;

    for (;;)
    {
        auto const chunk = socket_.readAll();
        if (chunk.isEmpty())
        {
            qDebug() << "Failed to read from server" << socket_.errorString();
            break;
        }

        n_bytes_read_ += chunk.size();
        qDebug() << Q_FUNC_INFO << "n_bytes_read is now" << n_bytes_read_ << "after reading" << chunk.size();
        if (file_.write(chunk) == -1)
            qDebug() << Q_FUNC_INFO << "file write failed" << file_.errorString();

        // THIS IS JUST FOR EXTRA DEBUG INFORMATION
        QCryptographicHash hash(QCryptographicHash::Sha1);
        hash.addData(chunk.data(), 100);
        qInfo() << "Hash: bytes total: " << n_bytes_read_ << " " << hash.result().toHex();
        // THIS IS JUST FOR EXTRA DEBUG INFORMATION
    }
}

void RestoreReader::finish()
{
    qDebug() << "Finishing";
    read_all();

    file_.flush();
    fsync(file_.handle());
    file_.close();

    QCoreApplication::exit();
}
