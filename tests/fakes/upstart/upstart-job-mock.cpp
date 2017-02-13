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

#include "upstart-defs.h"
#include "upstart-job-mock.h"
#include "UpstartMockAdaptor.h"

#include <QDebug>

using namespace testing;

UpstartJobMock::UpstartJobMock(QSharedPointer<UpstartMockAdaptor> const & upstart_adaptor, QObject* parent)
    : QObject(parent)
    , upstart_adaptor_(upstart_adaptor)
{
    job_paths_["test_job"] = "/com/test/path";
}

UpstartJobMock::~UpstartJobMock() = default;

namespace
{
QStringList get_params_from_app_uris(QString const &app_uris)
{
    QStringList ret;

    auto new_uris = app_uris;
    auto uris = new_uris.remove(QString("APP_URIS=")).split(' ');
    for (auto item : uris)
    {
        if (item.length() >= 3)
        {
            // remove ' at the beggining and end
            auto item_no_quotes = item.remove(0, 1);
            item_no_quotes.chop(1);
            ret << item_no_quotes;
        }
        else
        {
            qWarning() << "ERROR item " << item << " in APP_URIS " << app_uris << " is not valid";
            return QStringList();
        }
    }
    return ret;
}

QStringList get_process_args(QStringList const &env)
{
    for (auto item : env)
    {
        qDebug() << "OPTION " << item;
        if (item.startsWith("APP_URIS"))
        {
            qDebug() << "Item APP_URIS was found";
            return  get_params_from_app_uris(item);
        }
    }
    return QStringList();
}

QString get_app_id(QStringList const &env)
{
    for (auto item : env)
    {
        qDebug() << "OPTION " << item;
        if (item.startsWith("APP_ID"))
        {
            qDebug() << "Item APP_ID was found";
            auto new_item = item;
            return item.remove(QString("APP_ID="));;
        }
    }
    return QString();
}
} // namespace

QDBusObjectPath UpstartJobMock::Start(QStringList const &env, bool wait)
{
    qDebug() << "UpstartJobMock::Start called with parameter: " << env << "|" << wait;
    auto params = get_process_args(env);

    auto app_id = get_app_id(env);

    if (app_id.isEmpty())
    {
        sendErrorReply(QDBusError::InvalidArgs, QString("Failed starting job. Please check that the APP_ID env is valid: [%s]").arg(env.join(':')));
    }
    // arg[0] is the process, arg[1] is the directory where to execute the process
    if (params.size() != 2)
    {
        sendErrorReply(QDBusError::InvalidArgs, QString("Failed starting job. Please check that the APP_URIS env is valid: [%s]").arg(env.join(':')));
    }
    if (!start_process(app_id, params.at(0), params.at(1)))
    {
        sendErrorReply(QDBusError::InvalidArgs, QString("Failed starting job. Please check that the APP_URIS env is valid: [%s]").arg(env.join(':')));
    }
    return QDBusObjectPath(UPSTART_HELPER_INSTANCE_PATH);
}

void UpstartJobMock::Stop(QStringList const &env, bool wait)
{
     qDebug() << "UpstartJobMock::Stop called with parameter: " << env << "|" << wait;
     auto app_id = get_app_id(env);

     if (app_id.isEmpty())
     {
         sendErrorReply(QDBusError::InvalidArgs, QString("Failed stopping job. Please check that the APP_ID env is valid: [%s]").arg(env.join(':')));
         return;
     }
     auto iter = processes_.find(app_id);
     if (iter == processes_.end())
     {
         sendErrorReply(QDBusError::InvalidArgs, QString("Failed stopping job. Process for app_id was not found [%s]").arg(app_id));
         return;
     }

     (*iter)->terminate();
}

QList<QDBusObjectPath> UpstartJobMock::GetAllInstances() const
{
    qDebug() << "UpstartJobMock::GetAllInstances() was called";
    QList<QDBusObjectPath> ret;
    ret.push_back(QDBusObjectPath(UPSTART_HELPER_INSTANCE_PATH));
    ret.push_back(QDBusObjectPath(UPSTART_HELPER_MULTI_INSTANCE_PATH));
    return ret;
}

bool UpstartJobMock::start_process(QString const & app_id, QString const & path, QString const & cwd)
{
    auto new_process = QSharedPointer<QProcess>(new QProcess(this));

    // set the cwd
    new_process->setWorkingDirectory(cwd);

    /* uncomment this line to see keeper-service stdout/stderr in test logs
       NB: this is useful for manual debugging but not for automated tests
       because connecting stdout/stderr between processes keeps keeper-service
       from shutting down properly */
#if 0
    new_process->setProcessChannelMode(QProcess::ForwardedChannels);
#endif

    // start the process
    QProcess setVolume;
    new_process->start(path, QStringList());

    if (!new_process->waitForStarted())
    {
        qWarning() << "Error starting process: [" << path << "] with CWD: [" << cwd << "]";
        qWarning() << "ERROR MESSAGE: " << new_process->errorString();
        return false;
    }

    QString instance_name = QStringLiteral("INSTANCE=backup-helper::%1").arg(app_id);
    qDebug() << "Sending signal " << QStringList{"JOB=untrusted-helper", instance_name};
    Q_EMIT(upstart_adaptor_->EventEmitted("started", {"JOB=untrusted-helper", instance_name}));

    processes_[app_id] = new_process;
    auto on_finished = [this, new_process, instance_name, app_id](int exit_code, QProcess::ExitStatus /*exit_status*/)
    {
        qDebug() << "Process finished: " << new_process->pid() << " Exit code: " << exit_code;
        auto iter = processes_.find(app_id);
        if (iter != processes_.end())
        {
            processes_.erase(iter);
        }
        Q_EMIT(upstart_adaptor_->EventEmitted("stopped", {"JOB=untrusted-helper", instance_name}));
    };

    QObject::connect(new_process.data(),  static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), on_finished);

    return true;
}
