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

//static constexpr int UPLOAD_BUFFER_MAX_ {1024*16};
constexpr int UPLOAD_BUFFER_MAX_ = 64 * 1024;

RestoreReader::RestoreReader(qint64 fd, QString const & file_path, QObject * parent)
    : QObject(parent)
    , file_path_(file_path)
    , file_(file_path)
{
    socket_.setSocketDescriptor(fd);
    connect(&socket_, &QLocalSocket::readyRead, this, &RestoreReader::read_chunk);
    connect(&socket_, &QLocalSocket::disconnected, this, &RestoreReader::finish);
    connect(&file_, &QIODevice::bytesWritten, this, &RestoreReader::onSocketBytesWritten);
    if (!file_.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qFatal("Error opening file");
    }
}

void RestoreReader::read_chunk()
{
    if (socket_.bytesAvailable())
    {
        char buffer[UPLOAD_BUFFER_MAX_];
        int n_read = socket_.read(buffer, UPLOAD_BUFFER_MAX_);
        if (n_read < 0)
        {
            qDebug("Failed to read from server");
            return;
        }
        n_bytes_read_ += n_read;
        file_.write(buffer, n_read);
        qDebug() << "Hash: bytes total: " << n_bytes_read_;
    }
}

void RestoreReader::onSocketBytesWritten(int64_t bytes)
{
    qDebug() << "Wrote " << bytes << " bytes";
    read_chunk();
}

void RestoreReader::finish()
{
//    file_.write(bytes_read_);
    qDebug() << "Finishing";
    file_.close();
    QCoreApplication::exit();
}
