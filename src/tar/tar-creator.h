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
 *   Charles Kerr <charles.kerr@canoincal.com>
 */

#pragma once

#include <QtGlobal> // qint64
#include <QObject> // parent
#include <QStringList>
#include <QScopedPointer>

#include <vector>


class TarCreatorPrivate;
class TarCreator : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(TarCreator)

public:
    Q_DISABLE_COPY(TarCreator)

    TarCreator(const QStringList& files, bool compress, QObject* parent=nullptr);
    ~TarCreator();

    qint64 calculate_size() const;
    bool step(std::vector<char>& fillme);

private:
    QScopedPointer<TarCreatorPrivate> const d_ptr;
};
