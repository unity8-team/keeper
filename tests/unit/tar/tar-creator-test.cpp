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

#include "tests/utils/file-utils.h"

#include "tar/tar-creator.h"

#include <gtest/gtest.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>

#include <algorithm>
#include <array>
#include <cstdio>

class TarCreatorFixture: public ::testing::Test
{
protected:

    void SetUp() override
    {
        qsrand(time(nullptr));
    }

    void TearDown() override
    {
    }

};

/***
****
***/

TEST_F(TarCreatorFixture, Create)
{
    static constexpr int n_runs {5};

    for (int i=0; i<n_runs; ++i)
    {
        for (const auto compression_enabled : std::array<bool,2>{false, true})
        {
            // build a directory full of random files
            QTemporaryDir in;
            QDir indir(in.path());
            ASSERT_TRUE(FileUtils::fillTemporaryDirectory(in.path()));

            // create the tar creator
            EXPECT_TRUE(QDir::setCurrent(in.path()));
            QStringList files;
            for (auto file : FileUtils::getFilesRecursively(in.path()))
                files += indir.relativeFilePath(file);
            TarCreator tar_creator(files, compression_enabled);

            // simple sanity check on its size estimate
            const auto estimated_size = tar_creator.calculate_size();
            const auto filesize_sum = std::accumulate(
                files.begin(),
                files.end(),
                0,
                [](ssize_t sum, QString const& filename){return sum + QFileInfo(filename).size();}
            );
            if (!compression_enabled)
                ASSERT_GT(estimated_size, filesize_sum);

            // does it match the actual size?
            size_t actual_size {};
            std::vector<char> contents, step;
            while (tar_creator.step(step)) {
                contents.insert(contents.end(), step.begin(), step.end());
                actual_size += step.size();
            }
            ASSERT_EQ(estimated_size, actual_size);

            // untar it
            QTemporaryDir out;
            QDir outdir(out.path());
            QFile tarfile(outdir.filePath("tmp.tar"));
            tarfile.open(QIODevice::WriteOnly);
            tarfile.write(contents.data(), contents.size());
            tarfile.close();
            QProcess untar;
            untar.setWorkingDirectory(outdir.path());
            untar.start("tar", QStringList() << "xf" << tarfile.fileName());
            EXPECT_TRUE(untar.waitForFinished()) << qPrintable(untar.errorString());

            // compare it to the original
            EXPECT_TRUE(tarfile.remove());
            EXPECT_TRUE(FileUtils::compareDirectories(in.path(), out.path()));
        }
    }
}
