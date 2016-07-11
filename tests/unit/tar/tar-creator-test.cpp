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

#include "tar/tar-creator.h"

#include <gtest/gtest.h>

#include <QDebug>
#include <QString>
#include <QTemporaryDir>

#include <algorithm>
#include <cstdio>

class TarCreatorFixture: public ::testing::Test
{
protected:

};


TEST_F(TarCreatorFixture, HelloWorld)
{
}

TEST_F(TarCreatorFixture, CreateUncompressedFromSingleDirectoryOfFiles)
{
    static constexpr int n_runs = 10;
    static constexpr int max_files_per_test = 32;
    static constexpr int max_filesize = 1024*1024;

    qsrand(time(nullptr));

    for (int i=0; i<n_runs; ++i)
    {
        // build a directory full of random files
        QTemporaryDir sandbox;
        const auto n_files = std::max(1, (qrand() % max_files_per_test));
        QVector<DummyFile::Info> files (n_files);
        qint64 filesize_sum = 0;
        for (decltype(files.size()) j=0, n=files.size(); j!=n; ++j)
        {
            const auto filesize = qrand() % max_filesize;
            auto& file = files[j];
            file = DummyFile::create(sandbox.path(), filesize);
            filesize_sum += file.info.size();
        }

        // start the TarCreator
        QStringList filenames;
        for (const auto& file : files)
            filenames.append(file.info.absoluteFilePath());
        TarCreator tar_creator(filenames, false);

        // simple sanity check on its size estimate
        const auto estimated_size = tar_creator.calculate_size();
        ASSERT_GT(estimated_size, filesize_sum);

        // does it match the actual size?
        size_t actual_size {};
        std::vector<char> buf;
        while (tar_creator.step(buf)) {
            std::cerr << "step got " << buf.size() << " bytes" << std::endl;
            actual_size += buf.size();
        }
        ASSERT_EQ(estimated_size, actual_size);

        // FIXME: now extract and confirm the checksums are the same
    }
}

// FIXME: calculate compressed size

// FIXME: actually build the compressed tar and confirm the size eq calculated size

// FIXME: what happens when we pass in a directory name instead of an ordinary file. We need to confirm the subtree gets walked correctly
