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

    Q_PROPERTY(Helper::State state READ state NOTIFY state_changed)
    State state() const;

    // NB: bytes per second
    Q_PROPERTY(int speed READ speed NOTIFY speed_changed)
    int speed() const;

    // NB: range is [0.0 .. 1.0]
    Q_PROPERTY(float percentDone READ percent_done NOTIFY percent_done_changed)
    float percent_done() const;

    Q_PROPERTY(qint64 expectedSize READ expected_size WRITE set_expected_size NOTIFY expected_size_changed)
    qint64 expected_size() const;
    void set_expected_size(qint64 n_bytes);

    static void registerMetaTypes();

    using clock_func = std::function<time_t()>;
    static time_t real_clock() {return time(nullptr);}

Q_SIGNALS:
    void state_changed(Helper::State);
    void speed_changed(int);
    void percent_done_changed(float);
    void expected_size_changed(qint64);

protected:
    Helper(const clock_func& clock=real_clock, QObject *parent=nullptr);
    void set_state(State);
    void record_data_transferred(qint64 n_bytes);

private:
    QScopedPointer<HelperPrivate> const d_ptr;
};

Q_DECLARE_METATYPE(Helper::State)
