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

#include "tar/tar-creator.h"

#include <gtest/gtest.h>

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>

#include <algorithm>
#include <cstdio>

class TarCreatorFixture: public ::testing::Test
{
protected:

    struct FileInfo
    {
        QString filename;
        QByteArray hash;
        qint64 size;
    };

     FileInfo create_some_file(const QString& dir, qint64 filesize)
    {
        // build a file holding filesize random letters
        QTemporaryFile f(dir+"/tmpfile-XXXXXX");
        f.setAutoRemove(false);
        f.open();
        qint64 left = filesize;
        while(left > 0)
        {
            constexpr qint64 max_step = 1024;
            char buf[max_step];
            int this_step = std::min(max_step, left);
            for(int i=0; i<this_step; ++i)
                buf[i] = 'a' + char(qrand() % ('z'-'a'));
            f.write(buf, this_step);
            left -= this_step;
        }
        f.close();

        // build a FileInfo struct for the new file
        FileInfo info;
        info.filename = f.fileName();
        info.size = f.size();
        f.open();
        QCryptographicHash hash(QCryptographicHash::Sha1);
        hash.addData(&f);
        info.hash = hash.result();
        f.close();
        return info;
    }
};


TEST_F(TarCreatorFixture, HelloWorld)
{
}

TEST_F(TarCreatorFixture, CreateUncompressedFromSingleDirectoryOfFiles)
{
    constexpr int n_runs = 10;
    constexpr int max_files_per_test = 32;
    constexpr int max_filesize = 1024*1024;

    for (int i=0; i<n_runs; ++i)
    {
        // build a directory full of random files
        QTemporaryDir sandbox;
        auto files = std::vector<FileInfo>(qrand()%max_files_per_test);
        qint64 filesize_sum = 0;
        for (decltype(files.size()) j=0, n=files.size(); j!=n; ++j)
        {
            const auto filesize = qrand() % max_filesize;
            files[j] = create_some_file(sandbox.path(), filesize);
            filesize_sum += files[i].size;
        }

        // build the TarCreator
        QStringList filenames;
        for (const auto& file : files)
            filenames.append(file.filename);
        TarCreator tar_creator (filenames, false);

        // simple sanity check on its size estimate
        const auto estimated_size = tar_creator.calculate_size();
        ASSERT_GT(estimated_size, filesize_sum);
    }
}
