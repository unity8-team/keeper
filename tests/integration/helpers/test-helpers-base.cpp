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

void TestHelpersBase::focus_cb(const gchar* appid, gpointer user_data)
{
    g_debug("Focus Callback: %s", appid);
    auto _this = static_cast<TestHelpersBase*>(user_data);
    _this->last_focus_appid = appid;
}

void TestHelpersBase::resume_cb(const gchar* appid, gpointer user_data)
{
    g_debug("Resume Callback: %s", appid);
    auto _this = static_cast<TestHelpersBase*>(user_data);
    _this->last_resume_appid = appid;

    if (_this->resume_timeout > 0)
    {
        _this->pause(_this->resume_timeout);
    }
}

void TestHelpersBase::debugConnection()
{
    if (true)
    {
        return;
    }

    DbusTestBustle* bustle = dbus_test_bustle_new("test.bustle");
    dbus_test_service_add_task(service, DBUS_TEST_TASK(bustle));
    g_object_unref(bustle);

    DbusTestProcess* monitor = dbus_test_process_new("dbus-monitor");
    dbus_test_service_add_task(service, DBUS_TEST_TASK(monitor));
    g_object_unref(monitor);
}

void TestHelpersBase::startTasks()
{
    dbus_test_service_start_tasks(service);

    bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    g_dbus_connection_set_exit_on_close(bus, FALSE);
    g_object_add_weak_pointer(G_OBJECT(bus), (gpointer*)&bus);

    /* Make sure we pretend the CG manager is just on our bus */
    g_setenv("UBUNTU_APP_LAUNCH_CG_MANAGER_SESSION_BUS", "YES", TRUE);

    ASSERT_TRUE(ubuntu_app_launch_observer_add_app_focus(focus_cb, this));
    ASSERT_TRUE(ubuntu_app_launch_observer_add_app_resume(resume_cb, this));

    registry = std::make_shared<ubuntu::app_launch::Registry>();
}

void TestHelpersBase::SetUp()
{
    Helper::registerMetaTypes();

    /* Click DB test mode */
    g_setenv("TEST_CLICK_DB", "click-db-dir", TRUE);
    g_setenv("TEST_CLICK_USER", "test-user", TRUE);

    gchar* linkfarmpath = g_build_filename(CMAKE_SOURCE_DIR, "link-farm", NULL);
    g_setenv("UBUNTU_APP_LAUNCH_LINK_FARM", linkfarmpath, TRUE);
    g_free(linkfarmpath);

    g_setenv("XDG_DATA_DIRS", CMAKE_SOURCE_DIR, TRUE);
    g_setenv("XDG_CACHE_HOME", CMAKE_SOURCE_DIR "/libertine-data", TRUE);
    g_setenv("XDG_DATA_HOME", xdg_data_home_dir.path().toLatin1().data(), TRUE);

    qDebug() << "XDG_DATA_HOME ON SETUP is:" << xdg_data_home_dir.path();

    service = dbus_test_service_new(NULL);

    auto keeper_process = dbus_test_process_new(KEEPER_SERVICE_BIN);
    dbus_test_task_set_name(DBUS_TEST_TASK(keeper_process), "Keeper");
    dbus_test_service_add_task(service, DBUS_TEST_TASK(keeper_process));
    g_clear_object(&keeper_process);

    debugConnection();

    mock = dbus_test_dbus_mock_new("com.ubuntu.Upstart");

    DbusTestDbusMockObject* obj =
        dbus_test_dbus_mock_get_object(mock, UPSTART_PATH, UPSTART_INTERFACE, NULL);

    dbus_test_dbus_mock_object_add_method(mock, obj, "EmitEvent", G_VARIANT_TYPE("(sasb)"), NULL, "", NULL);

    dbus_test_dbus_mock_object_add_method(mock, obj, "GetJobByName", G_VARIANT_TYPE("s"), G_VARIANT_TYPE("o"),
                                          "if args[0] == 'application-click':\n"
                                          "	ret = dbus.ObjectPath('/com/test/application_click')\n"
                                          "elif args[0] == 'application-legacy':\n"
                                          "	ret = dbus.ObjectPath('/com/test/application_legacy')\n"
                                          "elif args[0] == 'untrusted-helper':\n"
                                          "	ret = dbus.ObjectPath('/com/test/untrusted/helper')\n",
                                          NULL);

    dbus_test_dbus_mock_object_add_method(mock, obj, "SetEnv", G_VARIANT_TYPE("(assb)"), NULL, "", NULL);

    /* Click App */
    DbusTestDbusMockObject* jobobj =
        dbus_test_dbus_mock_get_object(mock, "/com/test/application_click", UPSTART_JOB, NULL);

    dbus_test_dbus_mock_object_add_method(
        mock, jobobj, "Start", G_VARIANT_TYPE("(asb)"), NULL,
        "if args[0][0] == 'APP_ID=com.test.good_application_1.2.3':"
        "    raise dbus.exceptions.DBusException('Foo running', name='com.ubuntu.Upstart0_6.Error.AlreadyStarted')",
        NULL);

    dbus_test_dbus_mock_object_add_method(mock, jobobj, "Stop", G_VARIANT_TYPE("(asb)"), NULL, "", NULL);

    dbus_test_dbus_mock_object_add_method(mock, jobobj, "GetAllInstances", NULL, G_VARIANT_TYPE("ao"),
                                          "ret = [ dbus.ObjectPath('/com/test/app_instance') ]", NULL);

    DbusTestDbusMockObject* instobj =
        dbus_test_dbus_mock_get_object(mock, "/com/test/app_instance", UPSTART_INSTANCE, NULL);
    dbus_test_dbus_mock_object_add_property(mock, instobj, "name", G_VARIANT_TYPE_STRING,
                                            g_variant_new_string("com.test.good_application_1.2.3"), NULL);
    gchar* process_var = g_strdup_printf("[('main', %d)]", getpid());
    dbus_test_dbus_mock_object_add_property(mock, instobj, "processes", G_VARIANT_TYPE("a(si)"),
                                            g_variant_new_parsed(process_var), NULL);
    g_free(process_var);

    /*  Legacy App */
    DbusTestDbusMockObject* ljobobj =
        dbus_test_dbus_mock_get_object(mock, "/com/test/application_legacy", UPSTART_JOB, NULL);

    dbus_test_dbus_mock_object_add_method(mock, ljobobj, "Start", G_VARIANT_TYPE("(asb)"), NULL, "", NULL);

    dbus_test_dbus_mock_object_add_method(mock, ljobobj, "Stop", G_VARIANT_TYPE("(asb)"), NULL, "", NULL);

    dbus_test_dbus_mock_object_add_method(mock, ljobobj, "GetAllInstances", NULL, G_VARIANT_TYPE("ao"),
                                          "ret = [ dbus.ObjectPath('/com/test/legacy_app_instance') ]", NULL);

    DbusTestDbusMockObject* linstobj = dbus_test_dbus_mock_get_object(mock, "/com/test/legacy_app_instance",
                                                                      UPSTART_INSTANCE, NULL);
    dbus_test_dbus_mock_object_add_property(mock, linstobj, "name", G_VARIANT_TYPE_STRING,
                                            g_variant_new_string("multiple-2342345"), NULL);
    dbus_test_dbus_mock_object_add_property(mock, linstobj, "processes", G_VARIANT_TYPE("a(si)"),
                                            g_variant_new_parsed("[('main', 5678)]"), NULL);

    /*  Untrusted Helper */
    DbusTestDbusMockObject* uhelperobj =
        dbus_test_dbus_mock_get_object(mock, UNTRUSTED_HELPER_PATH, UPSTART_JOB, NULL);

    dbus_test_dbus_mock_object_add_method(mock, uhelperobj, "Start", G_VARIANT_TYPE("(asb)"), NULL,
                        "import os\n"
                        "import sys\n"
                        "import subprocess\n"
                        "target = open(\"/tmp/testHelper\", 'w')\n"
                        "exec_app=\"\"\n"
                        "for item in args[0]:\n"
                        "    keyVal = str(item)\n"
                        "    keyVal = keyVal.split(\"=\")\n"
                        "    if len(keyVal) == 2:\n"
                        "        os.environ[keyVal[0]] = keyVal[1]\n"
                        "        if keyVal[0] == \"APP_URIS\":\n"
                        "            exec_app = keyVal[1].replace(\"'\", '')\n"
                        "            target.write(exec_app)\n"
                        "            params = exec_app.split()\n"
                        "            if len(params) > 1:\n"
                        "                os.chdir(params[1])\n"
                        "                proc = subprocess.Popen(params[0], shell=True, stdout=subprocess.PIPE)\n"
                        "target.close\n"
                        , NULL);

    dbus_test_dbus_mock_object_add_method(mock, uhelperobj, "Stop", G_VARIANT_TYPE("(asb)"), NULL, "", NULL);

    dbus_test_dbus_mock_object_add_method(mock, uhelperobj, "GetAllInstances", NULL, G_VARIANT_TYPE("ao"),
                                          "ret = [ dbus.ObjectPath('/com/test/untrusted/helper/instance'), "
                                          "dbus.ObjectPath('/com/test/untrusted/helper/multi_instance') ]",
                                          NULL);

    DbusTestDbusMockObject* uhelperinstance = dbus_test_dbus_mock_get_object(
        mock, "/com/test/untrusted/helper/instance", UPSTART_INSTANCE, NULL);
    dbus_test_dbus_mock_object_add_property(mock, uhelperinstance, "name", G_VARIANT_TYPE_STRING,
                                            g_variant_new_string("untrusted-type::com.foo_bar_43.23.12"), NULL);

    DbusTestDbusMockObject* unhelpermulti = dbus_test_dbus_mock_get_object(
        mock, "/com/test/untrusted/helper/multi_instance", UPSTART_INSTANCE, NULL);
    dbus_test_dbus_mock_object_add_property(
        mock, unhelpermulti, "name", G_VARIANT_TYPE_STRING,
        g_variant_new_string("backup-helper:24034582324132:com.bar_foo_8432.13.1"), NULL);

    /* Create the cgroup manager mock */
    cgmock = dbus_test_dbus_mock_new("org.test.cgmock");
    g_setenv("UBUNTU_APP_LAUNCH_CG_MANAGER_NAME", "org.test.cgmock", TRUE);

    DbusTestDbusMockObject* cgobject = dbus_test_dbus_mock_get_object(cgmock, "/org/linuxcontainers/cgmanager",
                                                                      "org.linuxcontainers.cgmanager0_0", NULL);
    dbus_test_dbus_mock_object_add_method(cgmock, cgobject, "GetTasksRecursive", G_VARIANT_TYPE("(ss)"),
                                          G_VARIANT_TYPE("ai"), "ret = [100, 200, 300]", NULL);

    /* Put it together */
    dbus_test_service_add_task(service, DBUS_TEST_TASK(mock));
    dbus_test_service_add_task(service, DBUS_TEST_TASK(cgmock));
}

void TestHelpersBase::TearDown()
{
    registry.reset();

    ubuntu_app_launch_observer_delete_app_focus(focus_cb, this);
    ubuntu_app_launch_observer_delete_app_resume(resume_cb, this);

    g_clear_object(&mock);
    g_clear_object(&cgmock);
    g_clear_object(&service);

    g_object_unref(bus);

    unsigned int cleartry = 0;
    while (bus != NULL && cleartry < 100)
    {
        pause(100);
        cleartry++;
    }

    // if the test failed, keep the artifacts so devs can examine them
    QDir data_home_dir(CMAKE_SOURCE_DIR "/libertine-home");
    const auto passed = ::testing::UnitTest::GetInstance()->current_test_info()->result()->Passed();
    if (passed)
        data_home_dir.removeRecursively();
    else
        qDebug("test failed; leaving '%s'", data_home_dir.path().toUtf8().constData());

    ASSERT_EQ(nullptr, bus);

    // if the test passed, clean up the xdg_data_home_dir temp too
    xdg_data_home_dir.setAutoRemove(passed);

    // let's leave things clean
    EXPECT_TRUE(removeHelperMarkBeforeStarting());
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

bool TestHelpersBase::checkStorageFrameworkFiles(QStringList const & sourceDirs, bool compression)
{
    QStringList dirs = sourceDirs;

    while (dirs.size() > 0)
    {
        auto dir = dirs.takeLast();
        QString lastFile = getLastStorageFrameworkFile();
        if (lastFile.isEmpty())
        {
            qWarning() << "Did not found enough storage framework files";
            return false;
        }
        if (!compareTarContent (lastFile, dir, compression))
        {
            return false;
        }
        // remove the last file, so next iteration the last one is different
        QFile::remove(lastFile);
    }
    return true;
}

bool TestHelpersBase::checkLastStorageFrameworkFile (QString const & sourceDir, bool compression)
{
    QString lastFile = getLastStorageFrameworkFile();
    if (lastFile.isEmpty())
    {
        qWarning() << "Last file from storage framework is empty";
        return false;
    }
    return compareTarContent (lastFile, sourceDir, compression);
}

bool TestHelpersBase::compareTarContent (QString const & tarPath, QString const & sourceDir, bool compression)
{
    QTemporaryDir tempDir;

    qDebug() << "Comparing tar content for dir:" << sourceDir << "with tar:" << tarPath;

    QFileInfo checkFile(tarPath);
    if (!checkFile.exists())
    {
        qWarning() << "File:" << tarPath << "does not exist";
        return false;
    }
    if (!checkFile.isFile())
    {
        qWarning() << "Item:" << tarPath << "is not a file";
        return false;
    }
    if (!tempDir.isValid())
    {
        qWarning() << "Temporary directory:" << tempDir.path() << "is not valid";
        return false;
    }

    if( !extractTarContents(tarPath, tempDir.path()))
    {
        return false;
    }
    return FileUtils::compareDirectories(sourceDir, tempDir.path());
}

bool TestHelpersBase::extractTarContents(QString const & tarPath, QString const & destination, bool compression)
{
    QProcess tarProcess;
    QString tarParams = compression ? QString("-xzvf") : QString("-xvf");
    qDebug() << "Starting the process...";
    QString tarCmd = QString("tar -C %1 %2 %3").arg(destination).arg(tarParams).arg(tarPath);
    system(tarCmd.toStdString().c_str());
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

QString TestHelpersBase::getLastStorageFrameworkFile()
{
    QString last;

    QDir sf_dir;
    if(find_storage_framework_dir(sf_dir))
    {
        QStringList sortedFiles;
        QFileInfoList files = sf_dir.entryInfoList();
        for(auto& file : sf_dir.entryInfoList())
            if (file.isFile())
                sortedFiles << file.absoluteFilePath();

        // we detect the last file by name.
        // the file creation time had not enough precision
        sortedFiles.sort();
        if (sortedFiles.isEmpty())
            qWarning() << "ERROR: no files in" << sf_dir.path();
        else
            last = sortedFiles.last();
    }

    return last;
}

int TestHelpersBase::checkStorageFrameworkNbFiles()
{
    QDir sf_dir;
    auto exists = find_storage_framework_dir(sf_dir);

    return exists
        ? sf_dir.entryInfoList(QDir::Files).size()
        : -1;
}

bool TestHelpersBase::checkStorageFrameworkContent(QString const & content)
{
    QString lastFile = getLastStorageFrameworkFile();
    if (lastFile.isEmpty())
    {
        qWarning() << "Last file from the storage framework was not found";
        return false;
    }
    QFile storage_framework_file(lastFile);
    if(!storage_framework_file.open(QFile::ReadOnly))
    {
        qWarning() << "ERROR: opening file:" << lastFile;
        return false;
    }

    QString file_content = storage_framework_file.readAll();

    return file_content == content;
}

bool TestHelpersBase::removeHelperMarkBeforeStarting()
{
    QFile helper_mark(SIMPLE_HELPER_MARK_FILE_PATH);
    if (helper_mark.exists())
    {
        return helper_mark.remove();
    }
    return true;
}



bool TestHelpersBase::waitUntilHelperFinishes(QString const & app_id, int maxTimeout, int times)
{
    // TODO create a new mock for upstart that controls the lifecycle of the
    // helper process so we can do this in a cleaner way.
    QFile helper_mark(SIMPLE_HELPER_MARK_FILE_PATH);
    QElapsedTimer timer;
    timer.start();
    auto times_to_wait = times;
    bool finished = false;
    while (!timer.hasExpired(maxTimeout) && !finished)
    {
        if (helper_mark.exists())
        {
            if (--times_to_wait)
            {
                helper_mark.remove();
                timer.restart();
                qDebug() << "HELPER FINISHED, WAITING FOR" << times_to_wait << "MORE";
            }
            else
            {
                qDebug() << "ALL HELPERS FINISHED";
                finished = true;
            }
            sendUpstartHelperTermination(app_id);
        }
    }
    return finished;
}

void TestHelpersBase::sendUpstartHelperTermination(QString const &app_id)
{
    // send the upstart signal so keeper-service is aware of the helper termination
    DbusTestDbusMockObject* objUpstart =
        dbus_test_dbus_mock_get_object(mock, UPSTART_PATH, UPSTART_INTERFACE, NULL);

    QString eventInfoStr = QString("('stopped', ['JOB=untrusted-helper', 'INSTANCE=backup-helper::%1'])").arg(app_id.toStdString().c_str());
    dbus_test_dbus_mock_object_emit_signal(
        mock, objUpstart, "EventEmitted", G_VARIANT_TYPE("(sas)"),
        g_variant_new_parsed(
                eventInfoStr.toLocal8Bit().data()),
        NULL);
    g_usleep(100000);
    while (g_main_pending())
    {
        g_main_iteration(TRUE);
    }
}

QString TestHelpersBase::getUUIDforXdgFolderPath(QString const &path, QVariantDictMap const & choices) const
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

GVariant* TestHelpersBase::find_env(GVariant* env_array, const gchar* var)
{
    GVariant* retval = nullptr;

    for (int i=0, n=g_variant_n_children(env_array); i<n; i++)
    {
        GVariant* child = g_variant_get_child_value(env_array, i);
        const gchar* envvar = g_variant_get_string(child, nullptr);

        if (g_str_has_prefix(envvar, var))
        {
            if (retval != nullptr)
            {
                g_warning("Found the env var more than once!");
                g_variant_unref(retval);
                return nullptr;
            }

            retval = child;
        }
        else
        {
            g_variant_unref(child);
        }
    }

    if (!retval)
    {
        gchar* envstr = g_variant_print(env_array, FALSE);
        g_warning("Unable to find '%s' in '%s'", var, envstr);
        g_free(envstr);
    }

    return retval;
}

std::string TestHelpersBase::get_env(GVariant* env_array, const gchar* key)
{
    std::string value;

    GVariant* variant = find_env(env_array, key);
    if (variant != nullptr)
    {
        const char* cstr = g_variant_get_string(variant, nullptr);
        if (cstr != nullptr)
        {
            cstr = strchr(cstr, '=');
            if (cstr != nullptr)
                value = cstr + 1;
        }

        g_clear_pointer(&variant, g_variant_unref);
    }

    return value;
}

bool TestHelpersBase::have_env(GVariant* env_array, const gchar* key)
{
    GVariant* variant = find_env(env_array, key);
    bool found = variant != nullptr;
    g_clear_pointer(&variant, g_variant_unref);
    return found;
}

void TestHelpersBase::pause(guint time)
{
    if (time > 0)
    {
        GMainLoop* mainloop = g_main_loop_new(NULL, FALSE);

        g_timeout_add(time,
                      [](gpointer pmainloop) -> gboolean
                      {
                          g_main_loop_quit(static_cast<GMainLoop*>(pmainloop));
                          return G_SOURCE_REMOVE;
                      },
                      mainloop);

        g_main_loop_run(mainloop);

        g_main_loop_unref(mainloop);
    }

    while (g_main_pending())
    {
        g_main_iteration(TRUE);
    }
}
