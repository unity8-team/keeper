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
 *     Xavi Garcia <xavi.garcia.mena@gmail.com>
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

QFileInfo
create_dummy_file(QDir const& dir, qint64 filesize)
{
    // NB we want to exercise long filenames, but this cutoff length is arbitrary
    static constexpr int MAX_BASENAME_LEN {200};
    int filename_len = qrand() % MAX_BASENAME_LEN;
    QString basename;
    for (int i=0; i<filename_len; ++i)
        basename += ('a' + char(qrand() % ('z'-'a')));
    basename += QStringLiteral("-XXXXXX");
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
        int this_step = std::min(max_step, left);
        for(int i=0; i<this_step; ++i)
            buf[i] = 'a' + char(qrand() % ('z'-'a'));
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
calculate_checksum(QString const & filePath, QCryptographicHash::Algorithm hashAlgorithm)
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

QByteArray
calculate_checksum(QString const & filePath)
{
    return calculate_checksum(filePath, QCryptographicHash::Sha1);
}

bool
fill_directory_recusively(QDir const & dir,
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
            // create a new subdirectory
            auto newDirName = QString("Directory_%1").arg(n_subdirs_left);
            if (!dir.mkdir(newDirName))
            {
                qWarning() << "Error creating temporary directory" << newDirName << "under" << dir.absolutePath() << std::strerror(errno);
                return false;
            }

            // fill it
            QDir newDir(dir.absoluteFilePath(newDirName));
            --n_subdirs_left;
            fill_directory_recusively(newDir, max_filesize, n_files_left, n_subdirs_left);
        }
        else if (n_files_left > 0)
        {
            // create a new file
            const auto filesize = qrand() % max_filesize;
            const auto fileinfo = create_dummy_file(dir, filesize);
            if (!fileinfo.isFile())
            {
                return false;
            }

            --n_files_left;
        }
    }

    return true;
}

} // namespace

/***
****
***/

void
FileUtils::fillTemporaryDirectory(QString const & dir_path, int min_files, int max_files, int max_filesize, int max_dirs)
{
    const auto dir = QDir(dir_path);
    const auto n_files_wanted = min_files == max_files ? min_files : min_files + (qrand() % (max_files - min_files));
    const auto n_subdirs_wanted = max_dirs;

    int n_files_left {n_files_wanted};
    int n_subdirs_left {n_subdirs_wanted};

    while (n_files_left > 0)
        fill_directory_recusively(dir, max_filesize, n_files_left, n_subdirs_left);
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
    auto checksum2 = calculate_checksum(filePath1, QCryptographicHash::Md5);
    if (checksum1 != checksum2)
    {
        qWarning() << "Checksum for file:" << filePath1 << "differ";
    }
    return checksum1 == checksum2;
}

bool
FileUtils::compareDirectories(QString const & dir1Path, QString const & dir2Path)
{
    // we only check for files, not directories
    QDir dir1(dir1Path);
    if (!QDir::isAbsolutePath(dir1Path))
    {
        qWarning() << "Error comparing directories: path for directories must be absolute";
        return false;
    }

    if (!checkPathIsDir(dir1Path))
    {
        return false;
    }
    if (!checkPathIsDir(dir2Path))
    {
        return false;
    }

    auto filesDir1 = getFilesRecursively(dir1Path);
    auto filesDir2 = getFilesRecursively(dir2Path);
    filesDir1.sort();
    filesDir2.sort();

    if ( filesDir1.size() != filesDir2.size())
    {
        qWarning() << "Number of files in directories mismatch, dir \""
                   << dir1Path
                   <<  "\" has ["
                   << filesDir1.size()
                   << "], dir \""
                   << dir2Path
                   << "\" has [" << filesDir2.size() << "]";

        for (int i=0; i<filesDir1.size() || i<filesDir2.size(); ++i)
        {
            if (i<filesDir1.size())
                qWarning() << "A" << filesDir1[i];
            if (i<filesDir2.size())
                qWarning() << "B" << filesDir2[i];
        }

        return false;
    }
    for (auto file: filesDir1)
    {
        auto filePath2 = file;
        filePath2.remove(0, dir1Path.length());
        filePath2 = dir2Path + filePath2;
        if (!compareFiles(file, filePath2))
        {
            qWarning() << "File [" << file << "] and file [" << filePath2 << "] are not equal";
            return false;
        }
    }
    return true;
}

bool FileUtils::checkPathIsDir(QString const &dirPath)
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
