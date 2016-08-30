/*
 * Copyright 2013-2016 Canonical Ltd.
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
 *     Ted Gould <ted.gould@canonical.com>
 *     Xavi Garcia <xavi.garcia.mena@gmail.com>
 *     Charles Kerr <charles.kerr@canonical.com>
 */
#include "test-helpers-base.h"

#include <sys/types.h>
#include <signal.h>

using namespace QtDBusTest;
using namespace QtDBusMock;

TestHelpersBase::TestHelpersBase()
    :dbus_mock(dbus_test_runner)
{
}

void TestHelpersBase::start_tasks()
{
    try
    {
        keeper_service.reset(
                new QProcessDBusService(DBusTypes::KEEPER_SERVICE,
                                        QDBusConnection::SessionBus,
                                        KEEPER_SERVICE_BIN,
                                        QStringList()));
        keeper_service->start(dbus_test_runner.sessionConnection());
    }
    catch (std::exception const& e)
    {
        qWarning() << "Error starting keeper service " << e.what();
        throw;
    }

    try
    {
        upstart_service.reset(
                new QProcessDBusService(UPSTART_SERVICE,
                                        QDBusConnection::SessionBus,
                                        UPSTART_MOCK_BIN,
                                        QStringList()));
        upstart_service->start(dbus_test_runner.sessionConnection());
    }
    catch (std::exception const& e)
    {
        qWarning() << "Error starting upstart service " << e.what();
        throw;
    }

    system("dbus-send --session --dest=org.freedesktop.DBus --type=method_call --print-reply /org/freedesktop/DBus org.freedesktop.DBus.ListNames");
}

void TestHelpersBase::SetUp()
{
    Helper::registerMetaTypes();

    g_setenv("XDG_DATA_DIRS", CMAKE_SOURCE_DIR, true);
    g_setenv("XDG_CACHE_HOME", CMAKE_SOURCE_DIR "/libertine-data", true);
    g_setenv("XDG_DATA_HOME", xdg_data_home_dir.path().toLatin1().data(), true);

    qDebug() << "XDG_DATA_HOME ON SETUP is:" << xdg_data_home_dir.path();

    g_setenv("DBUS_SYSTEM_BUS_ADDRESS", dbus_test_runner.systemBus().toStdString().c_str(), true);
    g_setenv("DBUS_SESSION_BUS_ADDRESS", dbus_test_runner.sessionBus().toStdString().c_str(), true);

    dbus_test_runner.startServices();
}

void TestHelpersBase::TearDown()
{
    g_unsetenv("XDG_DATA_DIRS");
    g_unsetenv("XDG_CACHE_HOME");
    g_unsetenv("XDG_DATA_HOME");
    g_unsetenv("DBUS_SYSTEM_BUS_ADDRESS");
    g_unsetenv("DBUS_SESSION_BUS_ADDRESS");

    // if the test failed, keep the artifacts so devs can examine them
    QDir data_home_dir(CMAKE_SOURCE_DIR "/libertine-home");
    const auto passed = ::testing::UnitTest::GetInstance()->current_test_info()->result()->Passed();
    if (passed)
        data_home_dir.removeRecursively();
    else
        qDebug("test failed; leaving '%s'", data_home_dir.path().toUtf8().constData());

    // if the test passed, clean up the xdg_data_home_dir temp too
    xdg_data_home_dir.setAutoRemove(passed);

    // TODO investigate why the service gets to an state that does not listen to SIGTEM signals
    // If we don't make the following call at main.cpp handler.setupUnixSignalHandlers();
    // it exists fine.
    kill(keeper_service->pid(), SIGKILL);
}

bool TestHelpersBase::init_helper_registry(QString const& registry)
{
    // make a copy of the test registry file relative to our tmp XDG_DATA_HOME
    // $ cp $testname-registry.json $XDG_DATA_HOME/keeper/helper-registry.json

    QDir data_home{xdg_data_home_dir.path()};
    data_home.mkdir(PROJECT_NAME);
    QDir keeper_data_home{data_home.absoluteFilePath(PROJECT_NAME)};
    QFileInfo registry_file(registry);
    bool copied = QFile::copy(
        registry_file.absoluteFilePath(),
        keeper_data_home.absoluteFilePath(QStringLiteral(HELPER_REGISTRY_FILENAME))
    );

    return copied;
}

bool TestHelpersBase::check_storage_framework_files(QStringList const & source_dirs, bool compression)
{
    QStringList dirs = source_dirs;

    while (dirs.size() > 0)
    {
        auto dir = dirs.takeLast();
        QString last_file = get_last_storage_framework_file();
        if (last_file.isEmpty())
        {
            qWarning() << "Did not found enough storage framework files";
            return false;
        }
        if (!compare_tar_content (last_file, dir, compression))
        {
            return false;
        }
        // remove the last file, so next iteration the last one is different
        QFile::remove(last_file);
    }
    return true;
}

bool TestHelpersBase::compare_tar_content (QString const & tar_path, QString const & sourceDir, bool compression)
{
    QTemporaryDir temp_dir;

    qDebug() << "Comparing tar content for dir:" << sourceDir << "with tar:" << tar_path;

    QFileInfo check_file(tar_path);
    if (!check_file.exists())
    {
        qWarning() << "File:" << tar_path << "does not exist";
        return false;
    }
    if (!check_file.isFile())
    {
        qWarning() << "Item:" << tar_path << "is not a file";
        return false;
    }
    if (!temp_dir.isValid())
    {
        qWarning() << "Temporary directory:" << temp_dir.path() << "is not valid";
        return false;
    }

    if( !extract_tar_contents(tar_path, temp_dir.path()))
    {
        return false;
    }
    return FileUtils::compareDirectories(sourceDir, temp_dir.path());
}

bool TestHelpersBase::extract_tar_contents(QString const & tar_path, QString const & destination, bool compression)
{
    QProcess tar_process;
    QString tar_params = compression ? QString("-xzf") : QString("-xf");
    qDebug() << "Starting the process...";
    QString tar_cmd = QString("tar -C %1 %2 %3").arg(destination).arg(tar_params).arg(tar_path);
    system(tar_cmd.toStdString().c_str());
    return true;
}

namespace
{
    bool find_storage_framework_dir(QDir & dir)
    {
        auto path = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                           "storage-framework",
                                           QStandardPaths::LocateDirectory);

        if (path.isEmpty())
        {
            qWarning() << "ERROR: unable to find storage-framework directory";
            return false;
        }
        qDebug() << "storage framework directory is" << path;
        dir = QDir(path);

        return true;
    }
}

QString TestHelpersBase::get_last_storage_framework_file()
{
    QString last;

    QDir sf_dir;
    if(find_storage_framework_dir(sf_dir))
    {
        QStringList sorted_files;
        QFileInfoList files = sf_dir.entryInfoList();
        for(auto& file : sf_dir.entryInfoList())
            if (file.isFile())
                sorted_files << file.absoluteFilePath();

        // we detect the last file by name.
        // the file creation time had not enough precision
        sorted_files.sort();
        if (sorted_files.isEmpty())
            qWarning() << "ERROR: no files in" << sf_dir.path();
        else
            last = sorted_files.last();
    }

    return last;
}

int TestHelpersBase::check_storage_framework_nb_files()
{
    QDir sf_dir;
    auto exists = find_storage_framework_dir(sf_dir);

    return exists
        ? sf_dir.entryInfoList(QDir::Files).size()
        : -1;
}

bool TestHelpersBase::wait_for_all_tasks_have_action_state(QStringList const & uuids, QString const & action_state, QSharedPointer<DBusInterfaceKeeperUser> const & keeper_user_iface, int max_timeout)
{
    QElapsedTimer timer;
    timer.start();
    bool finished = false;
    while (!timer.hasExpired(max_timeout) && !finished)
    {
        auto state = keeper_user_iface->state();
        auto all_helpers_finished = true;
        for (auto uuid : uuids)
        {
            if (!check_task_has_action_state(state, uuid, action_state))
            {
                all_helpers_finished = false;
                break;
            }
        }
        finished = all_helpers_finished;
    }
    return finished;
}

bool TestHelpersBase::check_task_has_action_state(QVariantDictMap const & state, QString const & uuid, QString const & action_state)
{
    auto iter = state.find(uuid);
    if (iter == state.end())
        return false;

    auto iter_props = (*iter).find("action");
    if (iter_props == (*iter).end())
        return false;

    return (*iter_props).toString() == action_state;
}

QString TestHelpersBase::get_uuid_for_xdg_folder_path(QString const &path, QVariantDictMap const & choices) const
{
    for(auto iter = choices.begin(); iter != choices.end(); ++iter)
    {
        const auto& values = iter.value();
        auto iter_values = values.find("subtype");
        if (iter_values != values.end())
        {
            if (iter_values.value().toString() == path)
            {
                // got it
                return iter.key();
            }
        }
    }

    return QString();
}
