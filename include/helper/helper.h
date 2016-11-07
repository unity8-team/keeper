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

#include <keeper/errors.h>
#include <util/attributes.h>

#include <QObject>
#include <QScopedPointer>

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
    enum class State {NOT_STARTED, STARTED, CANCELLED, FAILED, DATA_COMPLETE, COMPLETE};

    Q_PROPERTY(Helper::State state READ state NOTIFY state_changed)
    State state() const __pure;

    virtual QString to_string(Helper::State state) const;

    // NB: range is [0.0 .. 1.0]
    Q_PROPERTY(float percent_done READ percent_done NOTIFY percent_done_changed)
    float percent_done() const __pure;

    // NB: units is bytes_per_second
    int speed() const __pure;

    qint64 expected_size() const __pure;
    void set_expected_size(qint64 n_bytes);

    static void registerMetaTypes();

    // returns timestamp in msec
    using clock_func = std::function<uint64_t()>;
    static clock_func default_clock;

    // life cycle control.
    virtual void start(QStringList const& urls);
    virtual void stop();

    static constexpr int MAX_UAL_WAIT_TIME = 5000;

Q_SIGNALS:
    void state_changed(Helper::State);
    void percent_done_changed(float);
    void error(KeeperError error);

protected:
    Helper(QString const & appid, const clock_func& clock=default_clock, QObject *parent=nullptr);
    virtual void set_state(State);
    virtual void on_helper_started();
    virtual void on_helper_finished();
    bool is_helper_running() const;
    void record_data_transferred(qint64 n_bytes);

private:

    QScopedPointer<HelperPrivate> const d_ptr;
};

Q_DECLARE_METATYPE(Helper::State)
