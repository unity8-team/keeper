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

#include <helper/helper.h>

#include <QDebug>

class HelperPrivate
{
public:

    using clock_func = Helper::clock_func;
    using State = Helper::State;

    HelperPrivate(
        Helper * helper,
        clock_func const & clock
    )
        : q_ptr{helper}
        , clock_{clock}
        , state_{State::NOT_STARTED}
        , speed_{}
        , percent_done_{}
        , size_{}
        , sized_{}
        , expected_size_{}
    {
    }

    ~HelperPrivate() =default;

    Q_DISABLE_COPY(HelperPrivate)

    State state() const
    {
        return state_;
    }

    void set_state(State state)
    {
        if (state_ != state)
        {
            qDebug() << "changing state of helper" << static_cast<void*>(this) << "from" << toString(state_) << "to" << toString(state);
            state_ = state;
            q_ptr->state_changed(state);
        }
    }

    int speed() const
    {
        return speed_;
    }

    float percent_done() const
    {
        return percent_done_;
    }

    void record_data_transferred(qint64 n_bytes)
    {
        size_ += n_bytes;
        sized_ += double(n_bytes);

        update_percent_done();
    }

    qint64 expected_size() const
    {
        return expected_size_;
    }

    void set_expected_size(qint64 expected_size)
    {
        if (expected_size_ != expected_size)
        {
            expected_size_ = expected_size;
            q_ptr->expected_size_changed(expected_size_);
            update_percent_done();
        }
    }

private:

    void update_percent_done()
    {
        setPercentDone(float(expected_size_ != 0 ? sized_ / double(expected_size_) : 0));
    }

    void setPercentDone(float percent_done)
    {
        if (int(100*percent_done_) != int(100*percent_done))
        {
            percent_done_ = percent_done;
            q_ptr->percent_done_changed(percent_done_);
        }
    }

    QString toString(Helper::State state)
    {
        auto ret = QStringLiteral("bug");

        switch (state)
        {
            case State::NOT_STARTED: ret = QStringLiteral("not-started"); break;
            case State::STARTED:     ret = QStringLiteral("started");     break;
            case State::CANCELLED:   ret = QStringLiteral("cancelled");   break;
            case State::FAILED:      ret = QStringLiteral("failed");      break;
            case State::COMPLETE:    ret = QStringLiteral("complete");    break;
        }

        return ret;
    }

    Helper * const q_ptr;
    clock_func clock_;
    State state_ {};
    int speed_ {};
    float percent_done_ {};
    qint64 size_ {};
    double sized_ {};
    qint64 expected_size_ {};
};

/***
****
***/

Helper::Helper(clock_func const& clock, QObject *parent)
    : QObject{parent}
    , d_ptr{new HelperPrivate{this, clock}}
{
}

Helper::~Helper() =default;

Helper::State
Helper::state() const
{
    Q_D(const Helper);

    return d->state();
}

int
Helper::speed() const
{
    Q_D(const Helper);

    return d->speed();
}

float
Helper::percent_done() const
{
    Q_D(const Helper);

    return d->percent_done();
}

void
Helper::record_data_transferred(qint64 n_bytes)
{
    Q_D(Helper);

    d->record_data_transferred(n_bytes);
}


void
Helper::set_state(State state)
{
    Q_D(Helper);

    d->set_state(state);
}

qint64
Helper::expected_size() const
{
    Q_D(const Helper);

    return d->expected_size();
}

void
Helper::set_expected_size(qint64 n_bytes)
{
    Q_D(Helper);

    d->set_expected_size(n_bytes);
}

void
Helper::registerMetaTypes()
{
    qRegisterMetaType<Helper::State>("Helper::State");
}
