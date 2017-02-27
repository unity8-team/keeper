/*
 * Copyright 2016 Canonical Ltd.
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
 *     Charles Kerr <charles.kerr@canonical.com>
 *     Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#include "tests/utils/file-utils.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QString>
#include <QTemporaryFile>

#include <cstring> // std::strerror


namespace
{

QString
create_dummy_string()
{
    // NB we want to exercise long filenames, but this cutoff length is arbitrary
    //static constexpr int MAX_BASENAME_LEN {200};
    static constexpr int MAX_BASENAME_LEN {10};
    auto const filename_len = std::max(10, qrand() % MAX_BASENAME_LEN);
    QString str;
    for (int i=0; i<filename_len; ++i)
        str += char(('a' + char(qrand() % ('z'-'a'))));
    return str;
}

QFileInfo
create_dummy_file(QDir const& dir, qint64 filesize)
{
    // get a filename
    auto basename = create_dummy_string() + QStringLiteral("-XXXXXX");
    auto template_name = dir.absoluteFilePath(basename);

    // fill the file with noise
    QTemporaryFile f(template_name);
    f.setAutoRemove(false);
    if(!f.open())
    {
        qWarning() << "Error opening temporary file:" << f.errorString();
        return QFileInfo();
    }
    static constexpr qint64 max_step = 1024;
    char buf[max_step];
    qint64 left = filesize;
    while(left > 0)
    {
        qint64 this_step = std::min(max_step, left);
        for(auto i=0; i<this_step; ++i)
            buf[i] = char('a' + char(qrand() % ('z'-'a')));
        if (f.write(buf, this_step) < this_step)
        {
            qWarning() << "Error writing to temporary file:" << f.errorString();
        }
        left -= this_step;
    }
    f.close();
    return QFileInfo(f.fileName());
}

QByteArray
calculate_checksum(QString const & filePath,
                   QCryptographicHash::Algorithm hashAlgorithm=QCryptographicHash::Sha1)
{
    QFile f(filePath);
    if (f.open(QFile::ReadOnly)) {
        QCryptographicHash hash(hashAlgorithm);
        if (hash.addData(&f)) {
            return hash.result();
        }
    }
    return QByteArray();
}

void
fill_directory_recursively(QDir const & dir,
                           int max_filesize,
                           int & n_files_left,
                           int & n_subdirs_left)
{
    // decide how many items to create in this directory
    const auto n_to_create = (n_files_left > 0)
                           ? std::max(1, qrand() % n_files_left)
                           : 0;

    for (auto i=0; i<n_to_create; ++i)
    {
        // decide if it's a file or a directory
        // we create a directory 25% of the time
        if ((n_subdirs_left > 0) && (qrand() % 100 < 25))
        {
            // create a new directory
            auto const newDirName = QStringLiteral("Directory_") + create_dummy_string();
            if (!dir.mkdir(newDirName))
            {
                qWarning() << "Error creating temporary directory" << newDirName << "under" << dir.absolutePath() << std::strerror(errno);
                return;
            }

            // fill it
            QDir newDir(dir.absoluteFilePath(newDirName));
            --n_subdirs_left;
            fill_directory_recursively(newDir, max_filesize, n_files_left, n_subdirs_left);
        }
        else if (n_files_left > 0)
        {
            // create a new file
            const auto filesize = qrand() % max_filesize;
            const auto fileinfo = create_dummy_file(dir, filesize);
            if (!fileinfo.isFile())
                return;

            --n_files_left;
        }
    }
}

} // unnamed namespace

/***
****
***/

void
FileUtils::fillTemporaryDirectory(QString const & dir_path,
                                  int min_files,
                                  int max_files,
                                  int max_filesize,
                                  int max_dirs)
{
    const auto dir = QDir(dir_path);
    const auto n_subdirs_wanted = max_dirs;
    const auto n_files_wanted = min_files == max_files
                              ? min_files
                              : min_files + (qrand() % (max_files - min_files));

    int n_files_left {n_files_wanted};
    int n_subdirs_left {n_subdirs_wanted};

    while (n_files_left > 0)
        fill_directory_recursively(dir, max_filesize, n_files_left, n_subdirs_left);
}

QStringList
FileUtils::getFilesRecursively(QString const & dirPath)
{
    QStringList ret;
    QDirIterator iter(dirPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    while(iter.hasNext())
    {
        QFileInfo info(iter.next());
        if (info.isFile())
        {
            ret << info.absoluteFilePath();
        }
        else if (info.isDir())
        {
            ret << getFilesRecursively(info.absoluteFilePath());
        }
    }

    return ret;
}

bool
FileUtils::copyDirsRecursively(QString const & source, QString const & dest)
{
    QFileInfo src_file_info(source);
    QFileInfo dest_file_info(dest);

    if (!src_file_info.isDir())
    {
        qWarning() << "Error copying directory " << source << ". It is not a directory";
        return false;
    }
    if (!dest_file_info.exists())
    {
        if (!QDir().mkdir(dest))
        {
            qWarning() << "Error creating destination directory: " << dest;
            return false;
        }
    }
    QDir source_dir(source);
    QStringList file_names = source_dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    for (const auto &file_name : file_names)
    {
        const QString new_src_file_path
                = source + QDir::separator() + file_name;
        const QString new_dest_file_path
                = dest + QDir::separator() + file_name;
        QFileInfo new_src_file_path_info(new_src_file_path);

        if (new_src_file_path_info.isDir())
        {
            if (!copyDirsRecursively(new_src_file_path, new_dest_file_path))
                return false;
        }
        else
        {
            if (!QFile::copy(new_src_file_path, new_dest_file_path))
            {
                qWarning() << "Error copying file " << new_src_file_path << " to " << new_dest_file_path;
                return false;
            }
        }
    }

    return true;
}

bool
FileUtils::clearDir(QString const &path)
{
    QDir dir(path);

    if (!dir.exists())
    {
        return false;
    }
    dir.setFilter(QDir::NoDotAndDotDot | QDir::Files);
    for(auto const & dirItem : dir.entryList())
    {
        if(!dir.remove(dirItem))
        {
            qWarning() << "Error removing directory: " << dirItem;
            return false;
        }
    }

    dir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
    for(auto const & dirItem : dir.entryList())
    {
        QDir subDir(dir.absoluteFilePath(dirItem));
        if (!subDir.removeRecursively())
        {
            qWarning() << "Error removing dir " <<  subDir.absolutePath() << " recursively";
            return false;
        }
    }

    return true;
}

bool
FileUtils::compareFiles(QString const & filePath1, QString const & filePath2)
{
    QFileInfo info1(filePath1);
    QFileInfo info2(filePath2);
    if (!info1.isFile())
    {
        qWarning() << "Origin file:" << info1.absoluteFilePath() << "does not exist";
        return false;
    }
    if (!info2.isFile())
    {
        qWarning() << "File to compare:" << info2.absoluteFilePath() << "does not exist";
        return false;
    }
    auto checksum1 = calculate_checksum(filePath1, QCryptographicHash::Md5);
    auto checksum2 = calculate_checksum(filePath2, QCryptographicHash::Md5);
    if (checksum1 != checksum2)
    {
        qWarning() << "Checksum for file:" << filePath1 << "differ";
    }
    return checksum1 == checksum2;
}

bool
FileUtils::compareDirectories(QString const & dir1Path, QString const & dir2Path)
{
    bool directories_identical = true;

    if (!QDir::isAbsolutePath(dir1Path))
    {
        qWarning() << Q_FUNC_INFO << "Error comparing directories: path for directories must be absolute";
        directories_identical = false;
    }
    else if (!checkPathIsDir(dir1Path))
    {
        qWarning() << Q_FUNC_INFO << dir1Path << "is not a directory";
        directories_identical = false;
    }
    else if (!checkPathIsDir(dir2Path))
    {
        qWarning() << Q_FUNC_INFO << dir2Path << "is not a directory";
        directories_identical = false;
    }
    else
    {
        QDir const dir1 {dir1Path};
        QDir const dir2 {dir2Path};

        // build a relative list of files under dir1
        QSet<QString> filesDir1;
        for (auto const& absolute_path : getFilesRecursively(dir1Path))
            filesDir1 << dir1.relativeFilePath(absolute_path);

        // build a relative list of files under dir2
        QSet<QString> filesDir2;
        for (auto const& absolute_path : getFilesRecursively(dir2Path))
            filesDir2 << dir2.relativeFilePath(absolute_path);

        for (auto const & relative: filesDir1 + filesDir2)
        {
            auto const abs1 = dir1.absoluteFilePath(relative);
            auto const abs2 = dir2.absoluteFilePath(relative);
            if (!compareFiles(abs1, abs2))
            {
                qWarning() << Q_FUNC_INFO << "files" << abs1 << "and" << abs2 << "are not equal";
                directories_identical = false;
                break;
            }
        }
    }

    return directories_identical;
}

bool
FileUtils::checkPathIsDir(QString const &dirPath)
{
    QFileInfo dirInfo = QFileInfo(dirPath);
    if (!dirInfo.isDir())
    {
        if (!dirInfo.exists())
        {
            qWarning() << "Directory: [" << dirPath << "] does not exist";
            return false;
        }
        else
        {
            qWarning() << "Path: [" << dirPath << "] is not a directory";
            return false;
        }
    }
    return true;
}
