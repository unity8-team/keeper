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

    void test_tar_creation(int min_files,
                           int max_files,
                           int max_filesize,
                           int max_dirs,
                           bool compressed,
                           int n_runs)
    {
        for (int i=0; i<n_runs; ++i)
        {
            QTemporaryDir in;
            QDir indir(in.path());
            FileUtils::fillTemporaryDirectory(in.path(), min_files, max_files, max_filesize, max_dirs);
            test_tar_creation(indir, compressed);
        }
    }

    void test_tar_creation(QDir const& in,
                           bool compression_enabled)
    {
        qDebug() << Q_FUNC_INFO;

        // create the tar creator
        EXPECT_TRUE(QDir::setCurrent(in.path()));
        QStringList files;
        for (auto& file : FileUtils::getFilesRecursively(in.path())) {
            qDebug() << file;
            files += in.relativeFilePath(file);
        }
        TarCreator tar_creator(files, compression_enabled);

        // test that the size calculator returns a consistent value
        const auto calculated_size = tar_creator.calculate_size();
        for (int i=0, n=5; i<n; ++i)
            EXPECT_EQ(calculated_size, tar_creator.calculate_size());

        // if uncompressed, test that the tar is at least as large as the source files
        if (!compression_enabled) {
            auto const filesize_sum = std::accumulate(
                files.begin(),
                files.end(),
                0,
                [](ssize_t sum, QString const& filename){return sum + QFileInfo(filename).size();}
            );
            EXPECT_GT(calculated_size, filesize_sum);
        }

        // create the tar
        size_t actual_size {};
        std::vector<char> contents, step;
        while (tar_creator.step(step)) {
            contents.insert(contents.end(), step.begin(), step.end());
            actual_size += step.size();
        }
        EXPECT_EQ(calculated_size, actual_size);

        // untar it
        QTemporaryDir out;
        QDir const outdir(out.path());
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
};

/***
****
***/

TEST_F(TarCreatorFixture, CreateUncompressedOfSingleFile)
{
    static constexpr int min_files {1};
    static constexpr int max_files {1};
    static constexpr int max_filesize {1024};
    static constexpr int max_dirs {0};
    static constexpr bool compressed {false};
    static constexpr int n_runs {5};

    test_tar_creation(min_files, max_files, max_filesize, max_dirs, compressed, n_runs);
}

TEST_F(TarCreatorFixture, CreateCompressedOfSingleFile)
{
    static constexpr int min_files {1};
    static constexpr int max_files {1};
    static constexpr int max_filesize {1024};
    static constexpr int max_dirs {0};
    static constexpr bool compressed {true};
    static constexpr int n_runs {5};

    test_tar_creation(min_files, max_files, max_filesize, max_dirs, compressed, n_runs);
}

TEST_F(TarCreatorFixture, CreateUncompressedOfTree)
{
    static constexpr int min_files {100};
    static constexpr int max_files {100};
    static constexpr int max_filesize {1024};
    static constexpr int max_dirs {10};
    static constexpr bool compressed {false};
    static constexpr int n_runs {5};

    test_tar_creation(min_files, max_files, max_filesize, max_dirs, compressed, n_runs);
}

TEST_F(TarCreatorFixture, CreateCompressedOfTree)
{
    static constexpr int min_files {100};
    static constexpr int max_files {100};
    static constexpr int max_filesize {1024};
    static constexpr int max_dirs {10};
    static constexpr bool compressed {true};
    static constexpr int n_runs {5};

    test_tar_creation(min_files, max_files, max_filesize, max_dirs, compressed, n_runs);
}


TEST_F(TarCreatorFixture, CreateCompressed)
{
    static constexpr int n_runs {5};

    for (int i=0; i<n_runs; ++i)
    {
        for (const auto compression_enabled : std::array<bool,1>{true})
        {
            // build a directory full of random files
            QTemporaryDir in;
            QDir indir(in.path());
            FileUtils::fillTemporaryDirectory(in.path());

            // create the tar creator
            EXPECT_TRUE(QDir::setCurrent(in.path()));
            QStringList files;
            for (auto file : FileUtils::getFilesRecursively(in.path()))
                files += indir.relativeFilePath(file);
            TarCreator tar_creator(files, compression_enabled);

            // is the size calculation repeatable?
            const auto calculated_size = tar_creator.calculate_size();
            for (int j=0; j<10; ++j)
                EXPECT_EQ(calculated_size, tar_creator.calculate_size());

            // simple sanity check on its size estimate
            const auto filesize_sum = std::accumulate(
                files.begin(),
                files.end(),
                0,
                [](ssize_t sum, QString const& filename){return sum + QFileInfo(filename).size();}
            );
            if (!compression_enabled)
                ASSERT_GT(calculated_size, filesize_sum);

            // does it match the actual size?
            size_t actual_size {};
            std::vector<char> contents, step;
            while (tar_creator.step(step)) {
                contents.insert(contents.end(), step.begin(), step.end());
                actual_size += step.size();
            }
            ASSERT_EQ(calculated_size, actual_size) << "compression_enabled" << compression_enabled;

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
