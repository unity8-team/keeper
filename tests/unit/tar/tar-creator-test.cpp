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

TEST_F(TarCreatorFixture, CreateUncompressed)
{
    static constexpr int n_runs = 10;

    qsrand(time(nullptr));

    for (int i=0; i<n_runs; ++i)
    {
        // build a directory full of random files
        QTemporaryDir sandbox;
        ASSERT_TRUE(FileUtils::fillTemporaryDirectory(sandbox.path()));
        const auto files = FileUtils::getFilesRecursively(sandbox.path());

        // create the tar creator
        TarCreator tar_creator(files, false);

        // simple sanity check on its size estimate
        const auto filesize_sum = std::accumulate(files.begin(), files.end(), 0, [](ssize_t sum, QString const& filename){return sum + QFileInfo(filename).size();});

#if 0
ssize_t filesize_sum {};
template< class InputIt, class T, class BinaryOperation >
T accumulate( InputIt first, InputIt last, T init,
              BinaryOperation op );
        for (const auto& file : files)
            filesize_sum += QFileInfo(file).size();
#endif
        const auto estimated_size = tar_creator.calculate_size();
        ASSERT_GT(estimated_size, filesize_sum);

        // does it match the actual size?
        size_t actual_size {};
        std::vector<char> buf;
        while (tar_creator.step(buf))
            actual_size += buf.size();
        ASSERT_EQ(estimated_size, actual_size);

        // FIXME: now extract and confirm the checksums are the same
    }
}

// FIXME: calculate compressed size

// FIXME: actually build the compressed tar and confirm the size eq calculated size

// FIXME: what happens when we pass in a directory name instead of an ordinary file. We need to confirm the subtree gets walked correctly
