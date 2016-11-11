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

RestoreReader::RestoreReader(qint64 fd, QString const & file_path, QObject * parent)
    : QObject(parent)
    , file_path_(file_path)
    , file_(file_path)
{
    socket_.setSocketDescriptor(fd);
    connect(&socket_, &QLocalSocket::readyRead, this, &RestoreReader::read_chunk);
    connect(&socket_, &QLocalSocket::disconnected, this, &RestoreReader::finish);
    if (!file_.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qFatal("Error opening file");
    }
}

void RestoreReader::read_chunk()
{
    qDebug() << "Reading chunk";
    auto bytes = socket_.readAll();
    qDebug() << "I read: " << bytes.size() << " bytes";
    n_bytes_read_ += bytes.size();
    qDebug() << "Total of: " << n_bytes_read_;
    file_.write(bytes);
}

void RestoreReader::finish()
{
    qDebug() << "Finishing";
    file_.close();
    QCoreApplication::exit();
}
