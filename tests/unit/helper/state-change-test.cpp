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

#include "state-change-test-manager.h"

#include <helper/helper.h>

#include <QSignalSpy>

#include <gtest/gtest.h>

#include <cmath> // std::pow()
#include <iostream>

TEST(HelperClass, TestStateSignalIsQueued)
{
    StateChangeTestManager helper_manager;

    QSignalSpy spy(&helper_manager.helper_, &Helper::state_changed);

    helper_manager.helper_.set_initial_state();

    spy.wait();
    EXPECT_EQ(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    EXPECT_EQ(qvariant_cast<Helper::State>(arguments.at(0)), Helper::State::STARTED);

    EXPECT_FALSE(helper_manager.error_when_setting_state_);
}
