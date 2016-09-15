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
 *   Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#pragma once

#include <helper/helper.h>

class StateTestHelper: public Helper
{
    Q_OBJECT

public:

    StateTestHelper(QString const & appid = QString(), clock_func const & clock = default_clock, QObject *parent = nullptr): Helper{appid, clock, parent} {}

    ~StateTestHelper() =default;

    void set_initial_state()
    {
        set_state(Helper::State::NOT_STARTED);
    }

    enum class TestState {VALUE_NOT_SET, VALUE_SET};
    TestState value_to_test = TestState::VALUE_NOT_SET;

protected:
    void set_state(Helper::State state) override
    {
        Helper::set_state(state);
        if (state == Helper::State::NOT_STARTED)
        {
            // change the state, if the signal would be
            // emitted direct the test manager would call the slot
            // before we call value_to_test = 1;
            set_state(Helper::State::STARTED);
            value_to_test = TestState::VALUE_SET;
        }
    }
};
