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
#include <QScopedPointer>

#include <ctime>
#include <functional>

class HelperPrivate;
class Helper : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Helper)

public:
    virtual ~Helper();
    Q_DISABLE_COPY(Helper)

    Q_ENUMS(State)
    enum class State {NOT_STARTED, STARTED, CANCELLED, FAILED, COMPLETE};

    Q_PROPERTY(Helper::State state READ state NOTIFY stateChanged)
    State state() const;

    // NB: bytes per second
    Q_PROPERTY(int speed READ speed NOTIFY speedChanged)
    int speed() const;

    // NB: range is [0.0 .. 1.0]
    Q_PROPERTY(float percentDone READ percentDone NOTIFY percentDoneChanged)
    float percentDone() const;

    static void registerMetaTypes();

    using clock_func = std::function<time_t()>;
    static time_t real_clock() {return time(nullptr);}

Q_SIGNALS:
    void stateChanged(Helper::State);
    void speedChanged(int);
    void percentDoneChanged(float);

protected:
    Helper(const clock_func& clock=real_clock, QObject *parent=nullptr);
    void setState(State);
    void recordDataTransferred(quint64 n_bytes);

private:
    QScopedPointer<HelperPrivate> const d_ptr;
};

Q_DECLARE_METATYPE(Helper::State)
