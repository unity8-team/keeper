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

#pragma once

#include <QDBusContext>
#include <QDBusObjectPath>
#include <QObject>
#include <QProcess>
#include <QSharedPointer>

class UpstartMockAdaptor;

namespace testing
{

class UpstartJobMock : public QObject, protected QDBusContext
{
    Q_OBJECT

public Q_SLOTS:
    QDBusObjectPath Start(QStringList const &env, bool wait);
    void Stop(QStringList const &env, bool wait);
    QList<QDBusObjectPath> GetAllInstances() const;

public:
    UpstartJobMock(QSharedPointer<UpstartMockAdaptor> const & upstart_adaptor, QObject* parent = 0);
    virtual ~UpstartJobMock();

Q_SIGNALS:
    void EventEmitted(QString const &name, QStringList const &env);
private:
    bool start_process(QString const & app_id, QString const & path, QString const & cwd);

    QMap<QString, QSharedPointer<QProcess>> processes_;
    QMap<QString, QString> job_paths_;
    QSharedPointer<UpstartMockAdaptor> upstart_adaptor_;
};

} // namespace testing

