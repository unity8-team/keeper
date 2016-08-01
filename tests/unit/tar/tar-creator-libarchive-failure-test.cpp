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

#include <archive.h>
#include <archive_entry.h>

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


namespace
{
    constexpr char const * WARNING_ERROR_MESSAGE {
        R"(It's probably something to do with..." I look up today's excuse ".. clock speed")"};
    constexpr char const * RETRY_ERROR_MESSAGE {
        R"(ELECTROMAGNETIC RADIATION FROM SATTELLITE DEBRIS)"};
    constexpr char const * FATAL_ERROR_MESSAGE {
        R"(I turn the page on the excuse sheet. "SOLAR FLARES" stares out at me.)"};

    const std::map<int,std::pair<int,const char*>> nth_call_errors = {
        { 1, std::make_pair(ARCHIVE_WARN, WARNING_ERROR_MESSAGE) },
        { 2, std::make_pair(ARCHIVE_RETRY, RETRY_ERROR_MESSAGE) },
        { 3, std::make_pair(ARCHIVE_FATAL, FATAL_ERROR_MESSAGE) }
    };

    int n_libarchive_calls {0};
    bool libarchive_fail_enabled {false};
}


class TarCreatorFixture: public ::testing::Test
{
protected:

    void SetUp() override
    {
        qsrand(time(nullptr));

        n_libarchive_calls = 0;
        libarchive_fail_enabled = false;
    }

    void TearDown() override
    {
    }

    void enable_libarchive_failure()
    {
        n_libarchive_calls = 0;
        libarchive_fail_enabled = true;
    }
};

// use ld -wrap to inject our versions of libarchive's API
// to inject failures that test tar-creator's error handling
extern "C"
{
    const char * __real_archive_error_string(struct archive *);
    int __real_archive_write_header(struct archive*, struct archive_entry *);

    const char * __wrap_archive_error_string(struct archive * archive)
    {
        auto it = nth_call_errors.find(n_libarchive_calls);
        return it == nth_call_errors.end()
            ? __real_archive_error_string(archive)
            : it->second.second;
    }

    int __wrap_archive_write_header(struct archive * archive, struct archive_entry * entry)
    {
        ++n_libarchive_calls;

        auto it = nth_call_errors.find(n_libarchive_calls);
        return it == nth_call_errors.end()
            ? __real_archive_write_header(archive, entry)
            : it->second.first;

    }
}

/***
****
***/

TEST_F(TarCreatorFixture, ArchiveWriteHeaderErrorInCalculateSize)
{
    enable_libarchive_failure();

    // build a directory full of random files
    QTemporaryDir in;
    QDir indir(in.path());
    ASSERT_TRUE(FileUtils::fillTemporaryDirectory(in.path()));

    // create the tar creator
    EXPECT_TRUE(QDir::setCurrent(in.path()));
    QStringList files;
    for (file : FileUtils::getFilesRecursively(in.path()))
        files += indir.relativeFilePath(file);
    TarCreator tar_creator(files, false);

    // confirm that archive_write_header() returning ARCHIVE_FAILURE
    // throws an exception that we can catch
    QString error_string;
    try {
        tar_creator.calculate_size();
    } catch (std::exception const& e) {
        error_string = QString::fromUtf8(e.what());
    }

    EXPECT_TRUE(error_string.contains(QString::fromUtf8(FATAL_ERROR_MESSAGE)))
        << qPrintable(error_string);
}

TEST_F(TarCreatorFixture, ArchiveWriteHeaderErrorInStep)
{
    enable_libarchive_failure();

    // build a directory full of random files
    QTemporaryDir in;
    QDir indir(in.path());
    ASSERT_TRUE(FileUtils::fillTemporaryDirectory(in.path()));

    // create the tar creator
    EXPECT_TRUE(QDir::setCurrent(in.path()));
    QStringList files;
    for (file : FileUtils::getFilesRecursively(in.path()))
        files += indir.relativeFilePath(file);
    TarCreator tar_creator(files, false);

    // build the tar blob
    QString error_string;
    std::vector<char> blob, step;
    try {
        while (tar_creator.step(step))
            blob.insert(blob.end(), step.begin(), step.end());
    } catch(std::exception& e) {
        error_string = QString::fromUtf8(e.what());
    }

    EXPECT_TRUE(error_string.contains(QString::fromUtf8(FATAL_ERROR_MESSAGE)))
        << qPrintable(error_string);
}
