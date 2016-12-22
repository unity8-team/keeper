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
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#pragma once

#include "client/keeper-errors.h"
#include "helper/metadata.h"

#include <QObject>
#include <QVector>

class MetadataProvider : public QObject
{
    Q_OBJECT
public:
    virtual ~MetadataProvider() =0;
    virtual QVector<Metadata> get_backups() const =0;
    virtual void get_backups_async() =0;

Q_SIGNALS:
    void finished(keeper::KeeperError error);

protected:
    explicit MetadataProvider(QObject *parent = nullptr) : QObject(parent){};
    QVector<Metadata> backups_;
};

inline MetadataProvider::~MetadataProvider() =default;
