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
 *   Xavi Garcia <xavi.garcia.mena@canoincal.com>
 *   Charles Kerr <charles.kerr@canoincal.com>
 */
#include "app-const.h" // DEKKO_APP_ID
#include "helper/metadata.h"
#include "keeper-task.h"

#include "private/keeper-task_p.h"

KeeperTaskPrivate::KeeperTaskPrivate(KeeperTask * keeper_task,
                  KeeperTask::TaskData const & task_data,
                  QSharedPointer<HelperRegistry> const & helper_registry,
                  QSharedPointer<StorageFrameworkClient> const & storage)
    : q_ptr(keeper_task)
    , task_data_(task_data)
    , helper_registry_(helper_registry)
    , storage_(storage)
{
}

KeeperTaskPrivate::~KeeperTaskPrivate() = default;

bool KeeperTaskPrivate::start()
{
    // initialize the helper
    q_ptr->init_helper();

    const auto urls = q_ptr->get_helper_urls();
    if (urls.isEmpty())
    {
        task_data_.action = helper_->to_string(Helper::State::FAILED);
        task_data_.error = "no helper information in registry";
        qWarning() << "ERROR: uuid: " << task_data_.metadata.uuid() << " has no url";
        calculate_and_notify_state(Helper::State::FAILED);
        return false;
    }

    // listen for helper state changes
    QObject::connect(helper_.data(), &Helper::state_changed,
        std::bind(&KeeperTaskPrivate::on_helper_state_changed, this, std::placeholders::_1)
    );

    // listen for helper process changes
    QObject::connect(helper_.data(), &Helper::percent_done_changed,
        std::bind(&KeeperTaskPrivate::on_helper_percent_done_changed, this, std::placeholders::_1)
    );

    helper_->start(urls);
    return true;
}

QVariantMap KeeperTaskPrivate::state() const
{
    return state_;
}

void KeeperTaskPrivate::set_current_task_action(QString const& action)
{
    task_data_.action = action;
    calculate_task_state();
}

void KeeperTaskPrivate::on_helper_state_changed(Helper::State state)
{
    switch (state)
    {
        case Helper::State::NOT_STARTED:
            break;

        case Helper::State::STARTED:
            qDebug() << "Helper started";
            break;

        case Helper::State::CANCELLED:
            qDebug() << "Helper cancelled... closing the socket.";
            break;

        case Helper::State::FAILED:
            qDebug() << "Helper failed... closing the socket.";
            break;

        case Helper::State::DATA_COMPLETE:
            task_data_.percent_done = 1;
            qDebug() << "Helper data complete.. closing the socket.";
            break;

        case Helper::State::COMPLETE:
            qDebug() << "Helper complete.";
            break;
    }
    set_current_task_action(helper_->to_string(state));
    calculate_and_notify_state(state);
}

void KeeperTaskPrivate::on_helper_percent_done_changed(float /*percent_done*/)
{
    calculate_task_state();
}

QVariantMap KeeperTaskPrivate::calculate_task_state()
{
    QVariantMap ret;

    auto const uuid = task_data_.metadata.uuid();

    ret.insert(QStringLiteral("action"), task_data_.action);

    ret.insert(QStringLiteral("display-name"), task_data_.metadata.display_name());

    // FIXME: assuming backup_helper_ for now...
    int32_t speed {};
    speed = helper_->speed();
    ret.insert(QStringLiteral("speed"), speed);

    task_data_.percent_done = helper_->percent_done();
    ret.insert(QStringLiteral("percent-done"), double(task_data_.percent_done));

    if (task_data_.action == "failed")
        ret.insert(QStringLiteral("error"), task_data_.error);

    ret.insert(QStringLiteral("uuid"), uuid);

    return ret;
}

void KeeperTaskPrivate::calculate_and_notify_state(Helper::State state)
{
    state_ = calculate_task_state();
    Q_EMIT(q_ptr->task_state_changed(state));
}

KeeperTask::KeeperTask(TaskData const & task_data,
                       QSharedPointer<HelperRegistry> const & helper_registry,
                       QSharedPointer<StorageFrameworkClient> const & storage,
                       QObject *parent)
    : QObject(parent)
    , d_ptr( new KeeperTaskPrivate(this, task_data, helper_registry, storage))
{
}

KeeperTask::KeeperTask(KeeperTaskPrivate & d, QObject *parent)
    : QObject(parent)
    , d_ptr(&d)
{
}


KeeperTask::~KeeperTask() = default;

bool KeeperTask::start()
{
    Q_D(KeeperTask);
    return d->start();
}

QVariantMap KeeperTask::state() const
{
    Q_D(const KeeperTask);
    return d->state();
}
