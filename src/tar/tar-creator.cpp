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

#include "tar/tar-creator.h"

class TarCreatorPrivate
{
public:

    TarCreatorPrivate(TarCreator* tar_creator, const QStringList& files, bool compress)
        : q_ptr(tar_creator)
        , files_(files)
        , compress_(compress)
    {
    }

    Q_DISABLE_COPY(TarCreatorPrivate)

    quint64 calculate_size()
    {
        return 0;
    }

private:

    TarCreator * const q_ptr {};
    const QStringList files_ {};
    const bool compress_ {};
};

/**
***
**/

TarCreator::TarCreator(const QStringList& files, bool compress, QObject* parent)
    : QObject(parent)
    , d_ptr(new TarCreatorPrivate(this, files, compress))
{
}

TarCreator::~TarCreator() =default;

quint64
TarCreator::calculate_size()
{
    Q_D(TarCreator);

    return d->calculate_size();
}
