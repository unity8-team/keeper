/*
 * Copyright 2013-2016 Canonical Ltd.
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
 *     Charles Kerr <charles.kerr@canonical.com>
 */
#pragma once

#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>

namespace StorageFrameworkLocalUtils
{
    bool find_storage_framework_dir(QDir & dir);

    bool find_storage_framework_root_dir(QDir & dir);

    int check_storage_framework_nb_files();

    bool check_storage_framework_files(QStringList const & source_dirs, bool compression=false);

    bool compare_tar_content (QString const & tar_path, QString const & source_dir, bool compression);

    bool extract_tar_contents(QString const & tar_path, QString const & destination, bool compression=false);

    QFileInfoList get_storage_framework_files();

    QString get_storage_framework_dir_name();

    bool get_storage_framework_file_equal_to(QString const & file_path, QString & path);

    bool get_storage_framework_file_equal_in_size_to(QString const & file_path, QString & path);
}
