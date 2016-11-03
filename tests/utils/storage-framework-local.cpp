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
#include "file-utils.h"
#include "storage-framework-local.h"

#include <storage-framework/storage_framework_client.h>

#include <QDebug>

namespace StorageFrameworkLocalUtils
{

bool find_storage_framework_root_dir(QDir & dir)
{
    auto path = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                       "storage-framework",
                                       QStandardPaths::LocateDirectory);

    if (path.isEmpty())
    {
        qWarning() << "ERROR: unable to find storage-framework directory";
        return false;
    }
    qDebug() << "storage framework directory is" << path;

    dir = QDir(QString("%1%2%3").arg(path).arg(QDir::separator()).arg(StorageFrameworkClient::KEEPER_FOLDER));

    if (!dir.exists())
    {
        return false;
    }
    qDebug() << "Keeper storage framework directory is" << dir.absolutePath();
    return true;
}

bool find_storage_framework_dir(QDir & dir)
{
    QDir sf_dir;
    if (!find_storage_framework_root_dir(sf_dir))
    {
        return false;
    }
    // return the first directory that we find here, in alphabetical order
    auto dirs_in_sf_folder = sf_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    if (!dirs_in_sf_folder.size())
    {
        qDebug() << "Error: no directory was found inside the keeper folder.";
        return false;
    }

    qDebug() << "The first directory containing data from storage framework is: " <<  dirs_in_sf_folder.at(0).absoluteFilePath();
    dir = QDir(dirs_in_sf_folder.at(0).absoluteFilePath());

    return true;
}

QFileInfoList get_storage_framework_files()
{
    QDir sf_dir;

    return find_storage_framework_dir(sf_dir)
        ? sf_dir.entryInfoList(QStringList{}, QDir::Files)
        : QFileInfoList{};
}

bool extract_tar_contents(QString const & tar_path, QString const & destination, bool compression)
{
    QProcess tar_process;
    QString tar_params = compression ? QString("-xzf") : QString("-xf");
    qDebug() << "Starting the process...";
    QString tar_cmd = QString("tar -C %1 %2 %3").arg(destination).arg(tar_params).arg(tar_path);
    system(tar_cmd.toStdString().c_str());
    return true;
}

bool compare_tar_content (QString const & tar_path, QString const & sourceDir, bool compression)
{
    QTemporaryDir temp_dir;

    qDebug() << "Comparing tar content for dir:" << sourceDir << "with tar:" << tar_path;

    QFileInfo check_file(tar_path);
    if (!check_file.exists())
    {
        qWarning() << "File:" << tar_path << "does not exist";
        return false;
    }
    if (!check_file.isFile())
    {
        qWarning() << "Item:" << tar_path << "is not a file";
        return false;
    }
    if (!temp_dir.isValid())
    {
        qWarning() << "Temporary directory:" << temp_dir.path() << "is not valid";
        return false;
    }

    if( !extract_tar_contents(tar_path, temp_dir.path()))
    {
        return false;
    }
    return FileUtils::compareDirectories(sourceDir, temp_dir.path());
}

bool check_storage_framework_files(QStringList const & source_dirs, bool compression)
{
    bool success {true};

    auto const backups = get_storage_framework_files();

    for (auto const& source : source_dirs)
    {
        bool backup_found {false};

        for (auto const& backup : backups)
        {
            auto const backup_filename = backup.absoluteFilePath();
            if ((backup_found = compare_tar_content (backup_filename, source, compression)))
            {
                qDebug() << Q_FUNC_INFO << "source" << source << "has match" << backup_filename;
                break;
            }
        }

        if (!backup_found)
        {
            qWarning() << Q_FUNC_INFO << "source" << source << "has no matching backup";
            success = false;
        }
    }

    return success;
}

int check_storage_framework_nb_files()
{
    QDir sf_dir;
    auto exists = find_storage_framework_dir(sf_dir);

    return exists
        ? sf_dir.entryInfoList(QDir::Files).size()
        : -1;
}

QString get_storage_framework_dir_name()
{
    QDir sf_dir;
    auto exists = find_storage_framework_dir(sf_dir);

    return exists
            ? sf_dir.dirName()
            : "";
}

} // namespace StorageFrameworkLocalUtils
