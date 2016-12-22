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
 *   Xavi Garcia <xavi.garcia.mena@canonical.com>
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#include "helper/metadata.h"
#include "keeper-task.h"

#include "private/keeper-task_p.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

KeeperTaskPrivate::KeeperTaskPrivate(KeeperTask * keeper_task,
                  KeeperTask::TaskData & task_data,
                  QSharedPointer<HelperRegistry> const & helper_registry,
                  QSharedPointer<StorageFrameworkClient> const & storage)
    : q_ptr(keeper_task)
    , task_data_(task_data)
    , helper_registry_(helper_registry)
    , storage_(storage)
    , error_(keeper::Error::OK)
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
        error_ = keeper::Error::HELPER_BAD_URL;
        qWarning() << QStringLiteral("Error: uuid %1 has no url").arg(task_data_.metadata.uuid());
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

    QObject::connect(helper_.data(), &Helper::error, [this](keeper::Error error){
        error_ = error;
    });

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
            qDebug() << "Helper cancelled";
            break;

        case Helper::State::FAILED:
            qDebug() << "Helper failed";
            break;

        case Helper::State::DATA_COMPLETE:
            qDebug() << "Helper data complete";
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
    calculate_and_notify_state(helper_->state());
}

QVariantMap KeeperTaskPrivate::calculate_task_state()
{
    QVariantMap ret;

    auto const uuid = task_data_.metadata.uuid();

    ret.insert(QStringLiteral("action"), task_data_.action);

    ret.insert(QStringLiteral("display-name"), task_data_.metadata.display_name());

    auto const speed = helper_->speed();
    ret.insert(QStringLiteral("speed"), int32_t(speed));

    auto const percent_done = helper_->percent_done();
    ret.insert(QStringLiteral("percent-done"), double(percent_done));

    if (task_data_.action == "failed" || task_data_.action == "cancelled")
    {
        auto error = error_;
        if (task_data_.error != keeper::Error::OK)
        {
            error = task_data_.error;
        }
        ret.insert(QStringLiteral("error"), QVariant::fromValue(error));
    }

    ret.insert(QStringLiteral("uuid"), uuid);

    QJsonDocument doc(QJsonObject::fromVariantMap(ret));
    qDebug() << QString(doc.toJson(QJsonDocument::Compact));

    return ret;
}

void KeeperTaskPrivate::calculate_and_notify_state(Helper::State state)
{
    recalculate_task_state();
    Q_EMIT(q_ptr->task_state_changed(state));
}

void KeeperTaskPrivate::recalculate_task_state()
{
    state_ = calculate_task_state();
}

void KeeperTaskPrivate::cancel()
{
    if (helper_)
    {
        helper_->stop();
    }
}

QVariantMap KeeperTaskPrivate::get_initial_state(KeeperTask::TaskData const &td)
{
    QVariantMap ret;

    auto const uuid = td.metadata.uuid();

    ret.insert(QStringLiteral("action"), td.action);

    // TODO review this when we add the restore tasks.
    // TODO we maybe have different fields
    ret.insert(QStringLiteral("display-name"), td.metadata.display_name());
    ret.insert(QStringLiteral("speed"), 0);
    ret.insert(QStringLiteral("percent-done"), double(0.0));
    ret.insert(QStringLiteral("uuid"), uuid);

    return ret;
}

QString KeeperTaskPrivate::to_string(Helper::State state)
{
    if (helper_)
    {
        return helper_->to_string(state);
    }
    else
    {
        qWarning() << "Asking for the string of a state when the helper is not initialized yet";
        return "bug";
    }
}

keeper::Error KeeperTaskPrivate::error() const
{
    return error_;
}

KeeperTask::KeeperTask(TaskData & task_data,
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

void KeeperTask::recalculate_task_state()
{
    Q_D(KeeperTask);

    return d->recalculate_task_state();
}


QVariantMap KeeperTask::get_initial_state(KeeperTask::TaskData const &td)
{
    return KeeperTaskPrivate::get_initial_state(td);
}

void KeeperTask::cancel()
{
    Q_D(KeeperTask);

    return d->cancel();
}

QString KeeperTask::to_string(Helper::State state)
{
    Q_D(KeeperTask);

    return d->to_string(state);
}

keeper::Error KeeperTask::error() const
{
    Q_D(const KeeperTask);

    return d->error();
}
