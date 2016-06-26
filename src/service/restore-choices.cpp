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

#include "service/restore-choices.h"

#include <QDebug>

RestoreChoices::RestoreChoices() =default;

RestoreChoices::~RestoreChoices() =default;

QVector<Metadata>
RestoreChoices::get_backups()
{
    QVector<Metadata> ret;

    // FIXME: need to walk through the 'Ubuntu Backups' directory's children, read the manifests, and use the info here

    // FIXME: async?

    return ret;
}
