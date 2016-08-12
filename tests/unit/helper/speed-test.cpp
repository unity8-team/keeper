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

#include "fake-helper.h"

#include <helper/helper.h>

#include <gtest/gtest.h>

#include <QDebug>


TEST(HelperClass, PercentDone)
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
qDebug() << "n_bytes" << n_bytes << "n_left" << n_left << "lhs" << (n_bytes-n_left)/n_bytes << "rhs" << helper.percent_done();
        EXPECT_EQ(int((10*(n_bytes-n_left)/n_bytes)), int(10*helper.percent_done()));
    }
}


TEST(HelperClass, Speed)
{
    uint64_t now_msec = time(nullptr);
    now_msec *= 100;
    auto my_clock = [&now_msec](){return now_msec;};

    TestHelper helper(my_clock);
    static constexpr qint64 n_bytes {100000};
    helper.set_expected_size(n_bytes);

    // no transfer yet, so speed should be zero
    EXPECT_EQ(0, helper.speed());

    static constexpr int n_trials {100};
    static constexpr int interval_seconds {5};
    static constexpr int MSEC_PER_SEC {1000};
    for(int i=0; i<n_trials; ++i)
    {
        // helper averages speed over time, so in order
        // to get a reliable decent test here let's
        // pretend to feed it data at a steady rate
        // over the next interval_seconds
        int expected_byte_per_second = qrand() % 1000000;
        for (int i=0; i<interval_seconds; ++i)
        {
            now_msec += MSEC_PER_SEC;
            helper.record_data_transferred(expected_byte_per_second);
        }
        EXPECT_EQ(expected_byte_per_second, helper.speed());
    }
}
