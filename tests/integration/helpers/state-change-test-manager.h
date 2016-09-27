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

#include "state-test-helper.h"

#include <QDebug>
#include <QObject>

class StateChangeTestManager : public QObject
{
    Q_OBJECT
public:
    explicit StateChangeTestManager(QObject * parent = nullptr)
        : QObject(parent)
    {
        connect(&helper_, &Helper::state_changed, this, &StateChangeTestManager::on_state_changed);
    };

    ~StateChangeTestManager() = default;

    void on_state_changed(Helper::State state)
    {
        qDebug() << "Checking if the value is set...";
        // what we test here is that the helper has the expected value
        // If the signal is emitted and the signals are called in DirectMode
        // we would receive this signal before the helper finished its handling of
        // the set_state method. In that case the value would be StateTestHelper::TestState::VALUE_NOT_SET...
        if (helper_.value_to_test != StateTestHelper::TestState::VALUE_SET)
        {
            qWarning() << "Wrong value detected in helper";
            error_when_setting_state_ = true;
        }
    }

    StateTestHelper helper_;
    bool error_when_setting_state_ = false;
};
