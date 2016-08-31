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
#include "upstart-mock.h"

#include <QDebug>

using namespace testing;

UpstartMock::UpstartMock(QObject* parent)
    : QObject(parent)
{
    job_paths_["test_job"] = "/com/test/path";
    job_paths_["untrusted-helper"] = "/com/test/untrusted/helper";
}

UpstartMock::~UpstartMock() = default;

void UpstartMock::EmitEvent(QString const &name, QStringList const & env, bool wait) const
{
    qDebug() << "EmitEvent called with parameter: " << name << "|" << env << "|" << wait;
}

QDBusObjectPath UpstartMock::GetJobByName(QString const &name) const
{
    qDebug() << "UpstartMock::GetJobByName(" << name << ")";

    QMap<QString, QString>::const_iterator iter = job_paths_.find(name);
    if (iter != job_paths_.end())
    {
        return QDBusObjectPath((*iter));
    }
    sendErrorReply(QDBusError::InvalidArgs, QString("Job %1 was not found").arg(name));
    return QDBusObjectPath();
}
