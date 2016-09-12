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
#include <string>


namespace
{
    constexpr char const * WARNING_ERROR_MESSAGE {
        "It's probably something to do with...\" I look up today's excuse \".. clock speed"};
    constexpr char const * RETRY_ERROR_MESSAGE {
        "ELECTROMAGNETIC RADIATION FROM SATELLITE DEBRIS"};
    constexpr char const * FATAL_ERROR_MESSAGE {
        "I turn the page on the excuse sheet. \"SOLAR FLARES\" stares out at me."};

    const std::map<int,std::pair<int,const char*>> nth_call_errors = {
        { 1, std::make_pair(ARCHIVE_WARN, WARNING_ERROR_MESSAGE) },
        { 2, std::make_pair(ARCHIVE_RETRY, RETRY_ERROR_MESSAGE) },
        { 3, std::make_pair(ARCHIVE_FATAL, FATAL_ERROR_MESSAGE) }
    };

    int n_libarchive_calls {0};
}


class TarCreatorFixture: public ::testing::Test
{
protected:

    void SetUp() override
    {
        qsrand(time(nullptr));

        n_libarchive_calls = 0;
    }

    void TearDown() override
    {
    }
};

// use ld -wrap to inject our versions of libarchive's API
// to inject failures that test tar-creator's error handling
extern "C"
{
    const char * __real_archive_error_string(struct archive *);
    int __real_archive_errno(struct archive *);
    int __real_archive_write_header(struct archive*, struct archive_entry *);
    ssize_t __real_archive_write_data(struct archive*, const void*, size_t);

    const char * __wrap_archive_error_string(struct archive * archive)
    {
        auto it = nth_call_errors.find(n_libarchive_calls);
        return it == nth_call_errors.end()
            ? __real_archive_error_string(archive)
            : it->second.second;
    }

    int __wrap_archive_errno(struct archive * archive)
    {
        auto it = nth_call_errors.find(n_libarchive_calls);

        return it == nth_call_errors.end()
            ? __real_archive_errno(archive)
            : it->second.first;
    }

    int __wrap_archive_write_header(struct archive * archive, struct archive_entry * entry)
    {
        ++n_libarchive_calls;

        auto it = nth_call_errors.find(n_libarchive_calls);
        return it == nth_call_errors.end()
            ? __real_archive_write_header(archive, entry)
            : it->second.first;

    }

    ssize_t __wrap_archive_write_data(struct archive * archive, const void* data, size_t n_bytes)
    {
        ++n_libarchive_calls;

        auto it = nth_call_errors.find(n_libarchive_calls);
        return it == nth_call_errors.end()
            ? __real_archive_write_data(archive, data, n_bytes)
            : -1;
    }
}

/***
****
***/

TEST_F(TarCreatorFixture, ArchiveWriteHeaderErrorInCalculateSize)
{
    // build a directory full of random files
    QTemporaryDir in;
    QDir indir(in.path());
    FileUtils::fillTemporaryDirectory(in.path(), 2);

    // create the tar creator
    EXPECT_TRUE(QDir::setCurrent(in.path()));
    QStringList files;
    for (auto file : FileUtils::getFilesRecursively(in.path()))
        files += indir.relativeFilePath(file);
    TarCreator tar_creator(files, false);

    // confirm that archive_write_header() returning ARCHIVE_FAILURE
    // throws an exception that we can catch
    std::string error_string;
    try {
        tar_creator.calculate_size();
    } catch (std::exception const& e) {
        error_string = e.what();
    }

    EXPECT_NE(std::string::npos, error_string.find(FATAL_ERROR_MESSAGE))
        << "expected: '" << FATAL_ERROR_MESSAGE << "'" << std::endl
        << "got: '" << error_string << "'" << std::endl;
}

TEST_F(TarCreatorFixture, ArchiveWriteHeaderErrorInStep)
{
    // build a directory full of random files
    QTemporaryDir in;
    QDir indir(in.path());
    FileUtils::fillTemporaryDirectory(in.path(), 2);

    // create the tar creator
    EXPECT_TRUE(QDir::setCurrent(in.path()));
    QStringList files;
    for (auto file : FileUtils::getFilesRecursively(in.path()))
        files += indir.relativeFilePath(file);
    TarCreator tar_creator(files, false);

    // build the tar blob
    std::string error_string;
    std::vector<char> blob, step;
    try {
        while (tar_creator.step(step))
            blob.insert(blob.end(), step.begin(), step.end());
    } catch(std::exception& e) {
        error_string = e.what();
    }

    EXPECT_NE(std::string::npos, error_string.find(FATAL_ERROR_MESSAGE))
        << "expected: '" << FATAL_ERROR_MESSAGE << "'" << std::endl
        << "got: '" << error_string << "'" << std::endl;
}
