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
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#include "tests/utils/dummy-file.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTemporaryFile>

DummyFile::Info
DummyFile::create(const QDir& dir, qint64 filesize)
{
    DummyFile::Info info;
    info.info = QFileInfo();
    info.checksum = QByteArray();

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
        qWarning() << "Error opening temporary file: " << f.errorString();
        return info;
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
            qWarning() << "Error writing to temporary file: " << f.errorString();
        }
        left -= this_step;
    }
    f.close();

    // get a checksum
    if(!f.open())
    {
        qWarning() << "Error opening temporary file: " << f.errorString();
        return info;
    }
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(&f);
    const auto checksum = hash.result();
    f.close();

    info.info = QFileInfo(f.fileName());
    info.checksum = checksum;
    return info;
}

bool recursiveFillDirectory(QString const & dirPath, int max_filesize, int & j, QVector<DummyFile::Info> & files, qint64 & filesize_sum, int & max_dirs)
{
    // get the number of files or directories that we will create at this level
    // it will always be less than the number of files remaining to create
    auto nb_items_to_create = qrand() % (files.size() - j);
    if ((files.size() - j) <= 1 )
    {
        nb_items_to_create = 1;
    }

    for (auto i = 0; i < nb_items_to_create && j < files.size(); ++i)
    {
        // decide if it's a file or a directory
        // we create a directory 25% of the time
        if (max_dirs && qrand() % 100 < 25)
        {
            QDir dir(dirPath);
            auto newDirName = QString("Directory_%1").arg(j);
            if (!dir.mkdir(newDirName))
            {
                qWarning() << "Error creating temporary directory " << newDirName << " under " << dirPath;
                return false;
            }

            QDir newDir(QString("%1%2%3").arg(dir.absolutePath()).arg(QDir::separator()).arg(newDirName));

            max_dirs--;

            // fill it
            recursiveFillDirectory(newDir.absolutePath(), max_filesize, j, files, filesize_sum, max_dirs);
        }
        else
        {
            // get the j file and increment the j index
            auto& file = files[j++];
            const auto filesize = qrand() % max_filesize;
            file = DummyFile::create(dirPath, filesize);
            if (file.info == QFileInfo())
            {
                return false;
            }
            filesize_sum += file.info.size();
        }
    }
    return true;
}

bool DummyFile::fillTemporaryDirectory(QString const & dir, int max_files_per_test, int max_filesize, int max_dirs)
{
    const auto n_files = std::max(1, (qrand() % max_files_per_test));
    QVector<DummyFile::Info> files (n_files);
    qint64 filesize_sum = 0;
    auto dirs_to_create = max_dirs;
    int j = 0;
    while (j<files.size())
    {
        recursiveFillDirectory(dir, max_filesize, j, files, filesize_sum, dirs_to_create);
    }
    return true;
}
