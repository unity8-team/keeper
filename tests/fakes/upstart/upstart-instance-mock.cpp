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
 *     Xavi Garcia Mena <xavi.garcia.mena@canonical.com>
 */
#include "upstart-instance-mock.h"

#include <QDebug>

using namespace testing;

UpstartInstanceMock::UpstartInstanceMock(QString const & name, QObject* parent)
    : QObject(parent)
    , name_(name)
{
}

UpstartInstanceMock::~UpstartInstanceMock() = default;

QString UpstartInstanceMock::name() const
{
    qDebug() << "Returning Instance name " << name_;
    return name_;
}

