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

namespace
{

QString toString(Helper::State state)
{
    QString ret = QStringLiteral("bug");

    switch (state)
    {
        case Helper::State::NOT_STARTED: ret = QStringLiteral("not-started"); break;
        case Helper::State::STARTED:     ret = QStringLiteral("started");     break;
        case Helper::State::CANCELLED:   ret = QStringLiteral("cancelled");   break;
        case Helper::State::FAILED:      ret = QStringLiteral("failed");      break;
        case Helper::State::COMPLETE:    ret = QStringLiteral("complete");    break;
    }

    return ret;
}

} // anon namespace

/***
****
***/

Helper::Helper(QObject *parent)
    : QObject(parent)
    , state_{State::NOT_STARTED}
{
}

Helper::~Helper() =default;

Helper::State
Helper::getState() const
{
    return state_;
}

void
Helper::setState(State state)
{
    if (state_ != state)
    {
        qDebug() << "changing state of helper" << static_cast<void*>(this) << "from" << toString(state_) << "to" << toString(state);

        state_ = state;

        Q_EMIT stateChanged(state);
    }
}
