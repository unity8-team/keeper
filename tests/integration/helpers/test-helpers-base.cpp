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
 *     Xavi Garcia <xavi.garcia.mena@canonical.com>
 *     Charles Kerr <charles.kerr@canonical.com>
 */
#include "test-helpers-base.h"

#include <sys/types.h>
#include <signal.h>

using namespace QtDBusTest;
using namespace QtDBusMock;

///
/// State helpers
//

bool qvariant_to_map(QVariant const& variant, QVariantMap& map)
{
    if (variant.type() == QMetaType::QVariantMap)
    {
        map = variant.toMap();
        return true;
    }
    qWarning() << Q_FUNC_INFO << ": Could not convert variant to QVariantMap. Variant received has type " << variant.typeName();

    return false;
}

bool qdbus_argument_to_variant_dict_map(QVariant const& variant, QVariantDictMap& map)
{
    if (variant.canConvert<QDBusArgument>())
    {
        QDBusArgument value(variant.value<QDBusArgument>());
        if (value.currentType() == QDBusArgument::MapType)
        {
            value >> map;
            return true;
        }
        else
        {
            qWarning() << Q_FUNC_INFO << ": Could not convert variant to QVariantDictMap. Variant received has type " << value.currentType();
        }
    }
    else
    {
        qWarning() << Q_FUNC_INFO << ": Could not convert variant to QDBusArgument.";
    }
    return false;
}

bool get_property_qvariant_dict_map(QString const & property, QVariant const &variant, QVariantDictMap & map)
{
    QVariantMap properties_map;
    if (!qvariant_to_map(variant, properties_map))
    {
        qWarning() << Q_FUNC_INFO << ": Error converting variant in PropertiesChanged signal to QVariantMap";
        return false;
    }

    auto iter = properties_map.find(property);
    if (iter == properties_map.end())
    {
        qWarning() << Q_FUNC_INFO << ": Property [" << property << "] was not found in PropertiesChanged";
        return false;
    }

    if(!qdbus_argument_to_variant_dict_map((*iter), map))
    {
        qWarning() << Q_FUNC_INFO << ": Error converting property [" << property << "] to QVariantDictMap";
        return false;
    }

    return true;
}

bool all_tasks_has_state(QMap<QString, QString> const & tasks_state, QString const & state)
{
    for (auto iter = tasks_state.begin(); iter != tasks_state.end(); ++iter )
    {
        if ((*iter) != state)
        {
            return false;
        }
    }
    return true;
}

bool get_task_property(QString const &property, QVariantMap const &values, QVariant &value)
{
    auto iter = values.find(property);
    if (iter == values.end())
    {
        qWarning() << Q_FUNC_INFO << ": Property [" << property << "] was not found.";
        return false;
    }
    value = (*iter);
    return true;
}

bool analyze_task_percentage_values(QString const & uuid, QList<QVariantMap> const & recorded_values)
{
    double previous_percentage = -1.0;
    for (auto iter = recorded_values.begin(); iter != recorded_values.end(); ++iter)
    {
        QVariant percentage;
        if (!get_task_property("percent-done", (*iter), percentage))
        {
            qWarning() << Q_FUNC_INFO << ": Percentage was not found for task: " << uuid;
            return false;
        }
        bool ok_double;
        auto percentage_double = percentage.toDouble(&ok_double);
        if (!ok_double)
        {
            qWarning() << Q_FUNC_INFO << ": Error converting percent-done to double for uuid: " << uuid << ". State: " << (*iter);
            return false;
        }
        if (percentage_double < previous_percentage)
        {
            qWarning() << Q_FUNC_INFO << ": Eror, current percentage is less than previous: current=" << percentage_double << ", previous=" << previous_percentage;
            return false;
        }
        previous_percentage = percentage_double;
    }
    return true;
}

bool check_valid_action_state_step(QString const &previous, QString const &current)
{
    if (current == "queued")
    {
        return previous == "none" || previous == "queued";
    }
    else if (current == "saving")
    {
        return previous == "saving" || previous == "queued";
    }
    else if (current == "finishing")
    {
        return previous == "finishing" || previous == "saving";
    }
    else if (current == "complete")
    {
        // we may pass from "saving" to "complete" if we don't have enough time
        // to emit the "finishing" state change
        return previous == "complete" || previous == "finishing" || previous == "saving";
    }
    else if (current == "failed")
    {
        // we can get to failed from any state except complete
        return previous != "complete";
    }
    else
    {
        // for possible new states, please add your code here
        qWarning() << "Unhandled state: " << current;
        return false;
    }
}

bool analyze_task_action_values(QString const & uuid, QList<QVariantMap> const & recorded_values)
{
    QString previous_action = "none";
    for (auto iter = recorded_values.begin(); iter != recorded_values.end(); ++iter)
    {
        QVariant action;
        if (!get_task_property("action", (*iter), action))
        {
            qWarning() << Q_FUNC_INFO << ": Action was not found for task: " << uuid;
            return false;
        }
        auto current_action = action.toString();

        if (!check_valid_action_state_step(previous_action, action.toString()))
        {
            qWarning() << Q_FUNC_INFO << ": Bad action state step: previous state=" << previous_action << ", current=" << action.toString();
            return false;
        }
        previous_action = action.toString();
    }
    return true;
}

bool analyze_task_display_name_values(QString const & uuid, QList<QVariantMap> const & recorded_values)
{
    QString previous_name;
    // check that the display name never changes between recorded states
    for (auto iter = recorded_values.begin(); iter != recorded_values.end(); ++iter)
    {
        QVariant name;
        if (!get_task_property("display-name", (*iter), name))
        {
            qWarning() << Q_FUNC_INFO << ": display-name was not found for task: " << uuid;
            return false;
        }
        auto current_name = name.toString();

        if (previous_name.isEmpty())
        {
            previous_name = name.toString();
        }
        else if(name.toString() != previous_name)
        {
            qWarning() << Q_FUNC_INFO << "ERROR: display-name for uuid: " << uuid << " changed from " << previous_name << " to " << name.toString();
            return false;
        }
    }
    return true;
}

bool analyze_tasks_values(QMap<QString, QList<QVariantMap>> const &uuids_state)
{
    for (auto iter = uuids_state.begin(); iter != uuids_state.end(); ++iter)
    {
        if(!analyze_task_display_name_values(iter.key(), (*iter)))
            return false;
        if(!analyze_task_action_values(iter.key(), (*iter)))
            return false;
        if(!analyze_task_percentage_values(iter.key(), (*iter)))
            return false;
    }
    return true;
}

bool verify_signal_interface_and_invalidated_properties(QVariant const &interface,
                                                        QVariant const &invalidated_properties,
                                                        QString const & expected_interface,
                                                        QString const & expected_property)
{
    auto props_list = invalidated_properties.toStringList();
    if (!props_list.contains(expected_property))
    {
        qWarning() << Q_FUNC_INFO << "ERROR: PropertiesChanged signal did not include the property: " << expected_property << " as the ones invalidated";
        return false;
    }

    if (interface.toString() != expected_interface)
    {
        qWarning() << Q_FUNC_INFO << "ERROR: Interface: [" << interface.toString() << "] is not valid. Expecting: [" << expected_interface << "]";
        return false;
    }
    return true;
}

///
///
///

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

#if 0
    // Enable this to get extra dbus information.
    if (!start_dbus_monitor())
    {
        throw std::logic_error("Error starting dbus-monitor process.");
    }
#endif
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

bool TestHelpersBase::wait_for_all_tasks_have_action_state(QStringList const & uuids, QString const & action_state, QSharedPointer<DBusInterfaceKeeperUser> const & keeper_user_iface, int max_timeout_msec)
{
    QElapsedTimer timer;
    timer.start();
    bool finished = false;
    while (!timer.hasExpired(max_timeout_msec) && !finished)
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

bool TestHelpersBase::capture_and_check_state_until_all_tasks_complete(QSignalSpy & spy, QStringList const & uuids, QString const & action_state, int max_timeout_msec)
{
    QMap<QString, QList<QVariantMap>> uuids_state;
    QMap<QString, QString> uuids_current_state;

    // initialize the current state map to wait for all expected uuids
    for (auto const& uuid : uuids)
    {
        uuids_current_state[uuid] = "none";
        uuids_state[uuid] = QList<QVariantMap>();
    }

    QElapsedTimer timer;
    timer.start();
    bool finished = false;
    while (!timer.hasExpired(max_timeout_msec) && !finished)
    {
        spy.wait();

        qDebug() << "PropertiesChanged SIGNALS RECEIVED:  " << spy.count();
        while (spy.count())
        {
            auto arguments = spy.takeFirst();

            if (arguments.size() != 3)
            {
                qWarning() << "Bad number of arguments in PropertiesChanged signal";
                return false;
            }

            // verify interface and invalidated_properties arguments
            if(!verify_signal_interface_and_invalidated_properties(arguments.at(0), arguments.at(2), DBusTypes::KEEPER_USER_INTERFACE, "State"))
            {
                return false;
            }
            QVariantDictMap keeper_state;
            if (!get_property_qvariant_dict_map("State", arguments.at(1), keeper_state))
            {
                return false;
            }
            for (auto iter = keeper_state.begin(); iter != keeper_state.end(); ++iter )
            {
                // check for unexpected uuids
                if (!uuids.contains(iter.key()))
                {
                    qWarning() << "State contains unexpected uuid: " << iter.key();
                    return false;
                }

                QVariantMap task_state;
                if (!qvariant_to_map((*iter), task_state))
                {
                    qWarning() << "Error converting second argument in PropertiesChanged signal to QVariantMap for uuid: " << iter.key();
                    return false;
                }
                qDebug() << "State for uuid: " << iter.key() << " : " << task_state;

                QVariant action;
                if (get_task_property("action", task_state, action))
                {
                    if (action.type() != QVariant::String)
                    {
                        qWarning() << "Property [action] is not a string";
                        return false;
                    }
                    // store the current action state
                    uuids_current_state[iter.key()] = action.toString();

                    bool store = true;
                    if (action.toString() == action_state)
                    {
                        // check if it worths storing this new event
                        store = uuids_state[iter.key()].last() != task_state;
                    }
                    if (store)
                    {
                        // store the current state for later inspection
                        uuids_state[iter.key()].push_back(task_state);
                    }
                }
            }
        }
        finished = all_tasks_has_state(uuids_current_state, action_state);
        if (finished)
        {
            qDebug() << "ALL TASKS FINISHED =========================================";
        }
    }
    // check for the recorded values
    return analyze_tasks_values(uuids_state);
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

bool TestHelpersBase::start_dbus_monitor()
{
    if (!dbus_monitor_process)
    {
        dbus_monitor_process.reset(new QProcess());

        dbus_monitor_process->setProcessChannelMode(QProcess::ForwardedChannels);

        dbus_monitor_process->start("dbus-monitor", QStringList() << "--session");
        if (!dbus_monitor_process->waitForStarted())
        {
            qWarning() << "Error, starting dbus-monitor: " << dbus_monitor_process->errorString();
            return false;
        }
    }
    else
    {
        qWarning() << "Error, dbus-monitor should be called one time per test.";
        return false;
    }
    return true;
}
