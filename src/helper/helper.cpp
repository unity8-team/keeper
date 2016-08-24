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

#include <ubuntu-app-launch/registry.h>
#include <service/app-const.h>
extern "C" {
    #include <ubuntu-app-launch.h>
}

#include <QDebug>

#include <cmath> // std::fabs()
#include <sys/time.h> // gettimeofday()


namespace
{

/**
 * Lazy-calculate transfer rates
 *
 * This class from libtransmission/bandwidth.c
 * is Copyright (C) 2008-2014 Mnemosyne LLC
 * and is available under GNU GPL versions 2 or 3.
 */
class RateHistory
{
public:

    RateHistory()
        : newest{}
        , transfers{}
        , cache_time{}
        , cache_val{}
    {
    }

    void
    add(uint64_t now, size_t size)
    {
        // find the right bin and update its size
        if (transfers[newest].date + GRANULARITY_MSEC >= now)
        {
            transfers[newest].size += size;
        }
        else
        {
            if (++newest == HISTORY_SIZE)
                newest = 0;
            transfers[newest].date = now;
            transfers[newest].size = size;
        }

        // invalidate cache
        cache_time = 0;
    }

    uint32_t
    speed_bytes_per_second(uint64_t now, unsigned int interval_msec=HISTORY_MSEC) const
    {
qDebug() << "now" << now << "cache_time" << cache_time << "interval_msec" << interval_msec;
        if (cache_time != now)
        {
            auto i = newest;
            uint64_t bytes {};
            uint64_t cutoff = now - interval_msec;

            for (;;)
            {
                if (transfers[i].date <= cutoff)
                    break;

                bytes += transfers[i].size;
qDebug() << "i" << i << "transfers[i] .date" << transfers[i].date << ".size" << transfers[i].size << "sum" << bytes;

                if (--i == -1) {
                    i = HISTORY_SIZE - 1; // circular history
qDebug() << "circular history";
                }

                if (i == newest) {
qDebug() << "i" << i << "transfers[i] .date" << transfers[i].date << ".size" << transfers[i].size << "sum" << bytes;
                    break; // we've come all the way around
                }
            }

            cache_val = uint32_t((bytes * 1000u) / interval_msec);
            cache_time = now;
qDebug() << "cache_val" << cache_val << "cache_time" << cache_time;
        }

        return cache_val;
    }

private:

    static constexpr int HISTORY_MSEC {2000};
    static constexpr int INTERVAL_MSEC = HISTORY_MSEC;
    static constexpr int GRANULARITY_MSEC = {200};
    static constexpr int HISTORY_SIZE = {INTERVAL_MSEC / GRANULARITY_MSEC};

    int newest;
    struct { uint64_t date, size; } transfers[HISTORY_SIZE];

    mutable uint64_t cache_time;
    mutable uint32_t cache_val;
};

} // anon namespace

/***
****
***/

class HelperPrivate
{
public:

    using clock_func = Helper::clock_func;
    using State = Helper::State;

    HelperPrivate(
        QString const & appid,
        Helper * helper,
        clock_func const & clock
    )
        : q_ptr{helper}
        , appid_{appid}
        , clock_{clock}
        , state_{State::NOT_STARTED}
        , size_{}
        , sized_{}
        , expected_size_{}
        , history_{}
        , registry_(new ubuntu::app_launch::Registry())
    {
        ual_init();
    }

    ~HelperPrivate()
    {
        ual_uninit();
    }

    Q_DISABLE_COPY(HelperPrivate)

    void set_state(State state)
    {
        if (state_ != state)
        {
            qDebug() << "changing state of helper" << static_cast<void*>(this) << "from" << toString(state_) << "to" << toString(state);
            state_ = state;
            q_ptr->state_changed(state);
        }
    }

    State state() const
    {
        return state_;
    }

    void set_expected_size(qint64 expected_size)
    {
        expected_size_ = expected_size;

        size_ = 0;
        sized_ = 0.0;
        update_percent_done();
    }

    qint64 expected_size() const
    {
        return expected_size_;
    }

    void record_data_transferred(qint64 n_bytes)
    {
        size_ += n_bytes;
        sized_ += double(n_bytes);

        history_.add(clock_(), size_t(n_bytes));

        update_percent_done();
    }

    int speed() const
    {
        return history_.speed_bytes_per_second(clock_());
    }

    float percent_done() const
    {
        return percent_done_;
    }

    void start(QStringList const& urls)
    {
        ual_start(urls);
    }

    void stop()
    {
        ual_stop();
    }

    void on_helper_process_stopped()
    {
        q_ptr->set_state(Helper::State::COMPLETE);
    }

private:
    /***
    ****  UAL
    ***/

    void ual_init()
    {
        ubuntu_app_launch_observer_add_helper_started(on_helper_started, HELPER_TYPE, this);
        ubuntu_app_launch_observer_add_helper_stop(on_helper_stopped, HELPER_TYPE, this);
    }

    void ual_uninit()
    {
        if (q_ptr->state() == Helper::State::STARTED)
            ual_stop();

        ubuntu_app_launch_observer_delete_helper_started(on_helper_started, HELPER_TYPE, this);
        ubuntu_app_launch_observer_delete_helper_stop(on_helper_stopped, HELPER_TYPE, this);
    }

    void ual_start(QStringList const& url_strings)
    {
        qDebug() << "Starting helper for app:" << appid_;

        std::vector<ubuntu::app_launch::Helper::URL> urls;
        for(const auto& url_string : url_strings) {
            qDebug() << "url" << url_string;
            urls.push_back(ubuntu::app_launch::Helper::URL::from_raw(url_string.toStdString()));
        }

        auto backupType = ubuntu::app_launch::Helper::Type::from_raw(HELPER_TYPE);

        auto appid = ubuntu::app_launch::AppID::parse(appid_.toStdString());
        auto helper = ubuntu::app_launch::Helper::create(backupType, appid, registry_);

        helper->launch(urls);
    }

    void ual_stop()
    {
        qDebug() << "Stopping helper for app:" << appid_;
        auto backupType = ubuntu::app_launch::Helper::Type::from_raw(HELPER_TYPE);

        auto appid = ubuntu::app_launch::AppID::parse(appid_.toStdString());
        auto helper = ubuntu::app_launch::Helper::create(backupType, appid, registry_);

        auto instances = helper->instances();

        if (instances.size() > 0 )
        {
            qDebug() << "We have instances";
            instances[0]->stop();
        }
    }

    static void on_helper_started(const char* appid, const char* /*instance*/, const char* /*type*/, void* vself)
    {
        qDebug() << "HELPER STARTED +++++++++++++++++++++++++++++++++++++" << appid;
        auto self = static_cast<HelperPrivate*>(vself);
        self->q_ptr->set_state(Helper::State::STARTED);
    }

    static void on_helper_stopped(const char* appid, const char* /*instance*/, const char* /*type*/, void* vself)
    {
        qDebug() << "HELPER STOPPED +++++++++++++++++++++++++++++++++++++" << appid;
        auto self = static_cast<HelperPrivate*>(vself);
        self->q_ptr->on_helper_process_stopped();
    }

    void update_percent_done()
    {
        percent_done_ = float(expected_size_ != 0 ? sized_ / double(expected_size_) : 0);

        const auto iold = int(last_notified_percent_done_*100);
        const auto inew = int(percent_done_*100);
        const auto is_noteworthy = inew != iold;

        if (is_noteworthy)
        {
            qDebug() << "emitting percent-done-changed" << percent_done_;
            Q_EMIT(q_ptr->percent_done_changed(percent_done_));
            last_notified_percent_done_ = percent_done_;
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
    QString appid_;
    clock_func clock_;
    State state_ {};
    qint64 size_ {};
    double sized_ {};
    qint64 expected_size_ {};
    RateHistory history_;
    float percent_done_ {};
    float last_notified_percent_done_ {};
    std::shared_ptr<ubuntu::app_launch::Registry> registry_;
};

/***
****
***/

Helper::Helper(QString const & appid, clock_func const& clock, QObject *parent)
    : QObject{parent}
    , d_ptr{new HelperPrivate{appid, this, clock}}
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

Helper::clock_func
Helper::default_clock = []()
{
    struct timeval tv;
    gettimeofday (&tv, nullptr);
    return uint64_t(tv.tv_sec*1000 + (tv.tv_usec/1000));
};

void
Helper::start(QStringList const& urls)
{
    Q_D(Helper);

    d->start(urls);
}

void
Helper::stop()
{
    Q_D(Helper);

    d->stop();
}

void
Helper::on_helper_process_stopped()
{
    Q_D(Helper);

    d->on_helper_process_stopped();
}
