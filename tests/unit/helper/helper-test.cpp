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

//#include "tests/utils/dummy-file.h"

//#include "tar/tar-creator.h"

#include "fake-helper.h"

#include <helper/helper.h>

#include <gtest/gtest.h>

//#include <QDebug>
//#include <QString>
//#include <QTemporaryDir>

//#include <algorithm>
//#include <cstdio>

class HelperFixture: public ::testing::Test
{
protected:

};


TEST_F(HelperFixture, HelloWorld)
{
}

TEST_F(HelperFixture, PercentDone)
{
    TestHelper helper;

    static constexpr qint64 n_bytes {1000};
    helper.set_expected_size(n_bytes);
    EXPECT_EQ(n_bytes, helper.expected_size());

    auto n_left = n_bytes;
    while (n_left > 0)
    {
        helper.record_data_transferred(1);
        --n_left;
        EXPECT_EQ(int((10*(n_bytes-n_left)/n_bytes)), int(10*helper.percent_done()));
    }
}
