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
 *   Xavi Garcia Mena <xavi.garcia.mena@canonical.com>
 */

#pragma once

#include <helper/metadata.h>

#include <QObject>

class ManifestPrivate;
class StorageFrameworkClient;

class Manifest : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Manifest)
public:
    Manifest(QSharedPointer<StorageFrameworkClient> const & storage, QString const & dir, QObject * parent = nullptr);
    virtual ~Manifest();
    Q_DISABLE_COPY(Manifest)

    void add_entry(Metadata const & entry);
    void store();

    void read();
    QVector<Metadata> get_entries();

    QString error() const;

Q_SIGNALS:
    void finished(bool success);

private:
    QScopedPointer<ManifestPrivate> const d_ptr;
};
