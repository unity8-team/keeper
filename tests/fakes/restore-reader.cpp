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
constexpr int UPLOAD_BUFFER_MAX_ = 16 * 1024;

RestoreReader::RestoreReader(qint64 fd, QString const & file_path, QObject * parent)
    : QObject(parent)
    , file_path_(file_path)
    , file_(file_path)
{
    socket_.setSocketDescriptor(fd);
    connect(&socket_, &QLocalSocket::readyRead, this, &RestoreReader::read_chunk);
    connect(&socket_, &QLocalSocket::disconnected, this, &RestoreReader::finish);
    connect(&file_, &QIODevice::bytesWritten, this, &RestoreReader::on_bytes_written);
    if (!file_.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qFatal("Error opening file");
    }
    if (socket_.bytesAvailable())
    {
        read_chunk();
    }
}

void RestoreReader::read_chunk()
{
    char buffer[UPLOAD_BUFFER_MAX_];
    int n_read = socket_.read(buffer, UPLOAD_BUFFER_MAX_);
    if (n_read < 0)
    {
        qDebug() << "Failed to read from server" << socket_.errorString();
        return;
    }
    n_bytes_read_ += n_read;
    file_.write(buffer, n_read);

    // THIS IS JUST FOR EXTRA DEBUG INFORMATION
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(buffer, 100);
    qDebug() << "Hash: bytes total: " << n_bytes_read_ << " " << hash.result().toHex();
    // THIS IS JUST FOR EXTRA DEBUG INFORMATION
}

void RestoreReader::on_bytes_written(int64_t bytes)
{
    if (socket_.bytesAvailable())
    {
        read_chunk();
    }
}

void RestoreReader::finish()
{
    qDebug() << "Finishing";
    auto avail = socket_.bytesAvailable();
    while (avail > 0)
    {
       read_chunk();
       avail = socket_.bytesAvailable();
    }
    file_.close();
    QCoreApplication::exit();
}
