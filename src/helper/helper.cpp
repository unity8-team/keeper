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
    {
        qDebug() << clock_();
    }

    ~HelperPrivate() =default;

    Q_DISABLE_COPY(HelperPrivate)

    State state() const
    {
        return state_;
    }

    void setState(State state)
    {
        if (state_ != state)
        {
            qDebug() << "changing state of helper" << static_cast<void*>(this) << "from" << toString(state_) << "to" << toString(state);
            state_ = state;
            q_ptr->stateChanged(state);
        }
    }

    int speed() const
    {
        return speed_;
    }

    float percentDone() const
    {
        return percent_done_;
    }

    void recordDataTransferred(quint64)
    {
        // FIXME
    }

private:

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
Helper::percentDone() const
{
    Q_D(const Helper);

    return d->percentDone();
}

void
Helper::recordDataTransferred(quint64 n_bytes)
{
    Q_D(Helper);

    d->recordDataTransferred(n_bytes);
}


void
Helper::setState(State state)
{
    Q_D(Helper);

    d->setState(state);
}

void
Helper::registerMetaTypes()
{
    qRegisterMetaType<Helper::State>("Helper::State");
}
