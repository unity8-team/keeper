/*
 * Copyright (C) 2016 Canonical, Ltd.
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
 *     Charles Kerr <charles.kerr@canonical.com>
 */

#pragma once

#include <QObject>

class Helper: public QObject
{
    Q_OBJECT

public:
    virtual ~Helper();
    Q_DISABLE_COPY(Helper)

    Q_ENUMS(State)
    enum class State {NOT_STARTED, STARTED, CANCELLED, FAILED, COMPLETE};

    Q_PROPERTY(Helper::State state READ getState NOTIFY stateChanged)
    State getState() const;

    static void registerMetaTypes();

Q_SIGNALS:
    void stateChanged(State new_state);

protected:
    Helper(QObject *parent=nullptr);
    void setState(State);

private:
    State state_;
};

Q_DECLARE_METATYPE(Helper::State)
