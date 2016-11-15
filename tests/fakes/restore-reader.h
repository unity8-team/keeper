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

#pragma once

#include <QObject>
#include <QFile>
#include <QLocalSocket>

class RestoreReader : public QObject
{
    Q_OBJECT
public:
    RestoreReader(qint64 fd, QString const & file_path, QObject * parent = nullptr);
    ~RestoreReader() = default;

public Q_SLOTS:
    void read_chunk();
    void finish();
    void onSocketBytesWritten(int64_t bytes);
private:
    QLocalSocket socket_;
    QString file_path_;
    qint64 n_bytes_read_ = 0;
    QFile file_;
    QByteArray bytes_read_;
};
