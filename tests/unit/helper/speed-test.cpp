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

#include <cmath> // std::pow()
#include <iostream>


TEST(HelperClass, PercentDone)
{
    TestHelper helper;

    static constexpr qint64 n_bytes {1000};
    helper.set_expected_size(n_bytes);
    EXPECT_EQ(n_bytes, helper.expected_size());

    auto n_left = n_bytes;
    static constexpr int checked_decimal_places {4};
    static const double test_multiplier = std::pow(10, checked_decimal_places);
    while (n_left > 0)
    {
        helper.record_data_transferred(1);
        --n_left;
        auto expect = std::round(test_multiplier * (n_bytes-n_left) / double(n_bytes));
        auto actual = std::round(test_multiplier * helper.percent_done());
        EXPECT_EQ(expect, actual);
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
