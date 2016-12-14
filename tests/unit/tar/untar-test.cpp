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
#include "tar/untar.h"

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

class UntarFixture: public ::testing::Test
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

TEST_F(UntarFixture, Untar)
{
    static constexpr int n_runs {5};
    static constexpr std::array<size_t,4> step_sizes = { 1024, 2048, 4096, INT_MAX };
    //static constexpr std::array<int,1> step_sizes = { 1024 };

    for (int i=0; i<n_runs; ++i)
    {
        // build a directory full of random files
        QTemporaryDir in;
        QDir indir(in.path());
        //FileUtils::fillTemporaryDirectory(in.path());
        FileUtils::fillTemporaryDirectory(in.path(), 3, 3, 4096, 1);

        // tar it up
        std::vector<char> contents;
        {
            EXPECT_TRUE(QDir::setCurrent(in.path()));
            QStringList files;
            for (auto file : FileUtils::getFilesRecursively(in.path()))
                files += indir.relativeFilePath(file);
            TarCreator tar_creator(files, false);
            std::vector<char> step;
            while (tar_creator.step(step))
                contents.insert(contents.end(), step.begin(), step.end());
        }

        // walk through an untar test for each of the step sizes
        for (auto const& step_size : step_sizes)
        {
            char const * walk = &contents.front();
            auto n_left = contents.size();

            // untar it
            QTemporaryDir out;
            QDir outdir(out.path());

            {
                Untar untar(out.path().toStdString());
                do
                {
                    auto const current_step_size = std::min(step_size, n_left);
                    EXPECT_TRUE(untar.step(walk, current_step_size));
                    n_left -= current_step_size;
                    walk += current_step_size;
                }
                while(n_left > 0);
            }

            // compare it to the original
            EXPECT_TRUE(FileUtils::compareDirectories(in.path(), out.path()));

            // if the test failed, keep the outdir for manual inspection
            auto const passed = ::testing::UnitTest::GetInstance()->current_test_info()->result()->Passed();
            out.setAutoRemove(passed);
        }

        // if the test failed, keep the indir for manual inspection
        auto const passed = ::testing::UnitTest::GetInstance()->current_test_info()->result()->Passed();
        in.setAutoRemove(passed);
    }
}
