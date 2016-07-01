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

#include "tar/tar-builder.h"

class TarBuilderPrivate
{
    Q_DISABLE_COPY(TarBuilderPrivate)
    Q_DECLARE_PUBLIC(TarBuilder)

    TarBuilder * const q_ptr {};
    const QStringList files_ {};
    const bool compress_ {};

    TarBuilderPrivate(TarBuilder* tar_builder, const QStringList& files, bool compress)
        : q_ptr(tar_builder)
        , files_(files)
        , compress_(compress)
    {
    }

    quint64 calculate_size()
    {
        return 0;
    }
};

/**
***
**/

TarBuilder::TarBuilder(const QStringList& files, bool compress, QObject* parent)
    : QObject(parent)
    , d_ptr(new TarBuilderPrivate(this, files, compress))
{
}

TarBuilder::~TarBuilder() =default;

quint64
TarBuilder::calculate_size()
{
    Q_D(TarBuilder);

    return d->calculate_size();
}
