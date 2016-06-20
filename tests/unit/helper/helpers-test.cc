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
 */

#include <future>
#include <thread>

#include <QCoreApplication>
#include <QSignalSpy>

#include <gtest/gtest.h>
#include <gio/gio.h>
#include <ubuntu-app-launch/registry.h>
#include <libdbustest/dbus-test.h>

#include "mir-mock.h"
#include <helper/backup-helper.h>



extern "C" {
    #include <ubuntu-app-launch.h>
    #include <fcntl.h>
}

class LibUAL : public ::testing::Test
{
protected:
    DbusTestService* service = NULL;
    DbusTestDbusMock* mock = NULL;
    DbusTestDbusMock* cgmock = NULL;
    GDBusConnection* bus = NULL;
    std::string last_focus_appid;
    std::string last_resume_appid;
    guint resume_timeout = 0;
    std::shared_ptr<ubuntu::app_launch::Registry> registry;

private:
    static void focus_cb(const gchar* appid, gpointer user_data)
    {
        g_debug("Focus Callback: %s", appid);
        LibUAL* _this = static_cast<LibUAL*>(user_data);
        _this->last_focus_appid = appid;
    }

    static void resume_cb(const gchar* appid, gpointer user_data)
    {
        g_debug("Resume Callback: %s", appid);
        LibUAL* _this = static_cast<LibUAL*>(user_data);
        _this->last_resume_appid = appid;

        if (_this->resume_timeout > 0)
        {
            _this->pause(_this->resume_timeout);
        }
    }

protected:
    /* Useful debugging stuff, but not on by default.  You really want to
       not get all this noise typically */
    void debugConnection()
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

    virtual void SetUp()
    {
        /* Click DB test mode */
        g_setenv("TEST_CLICK_DB", "click-db-dir", TRUE);
        g_setenv("TEST_CLICK_USER", "test-user", TRUE);

        gchar* linkfarmpath = g_build_filename(CMAKE_SOURCE_DIR, "link-farm", NULL);
        g_setenv("UBUNTU_APP_LAUNCH_LINK_FARM", linkfarmpath, TRUE);
        g_free(linkfarmpath);

        g_setenv("XDG_DATA_DIRS", CMAKE_SOURCE_DIR, TRUE);
        g_setenv("XDG_CACHE_HOME", CMAKE_SOURCE_DIR "/libertine-data", TRUE);
        g_setenv("XDG_DATA_HOME", CMAKE_SOURCE_DIR "/libertine-home", TRUE);

        service = dbus_test_service_new(NULL);

        debugConnection();

        mock = dbus_test_dbus_mock_new("com.ubuntu.Upstart");

        DbusTestDbusMockObject* obj =
            dbus_test_dbus_mock_get_object(mock, "/com/ubuntu/Upstart", "com.ubuntu.Upstart0_6", NULL);

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
            dbus_test_dbus_mock_get_object(mock, "/com/test/application_click", "com.ubuntu.Upstart0_6.Job", NULL);

        dbus_test_dbus_mock_object_add_method(
            mock, jobobj, "Start", G_VARIANT_TYPE("(asb)"), NULL,
            "if args[0][0] == 'APP_ID=com.test.good_application_1.2.3':"
            "    raise dbus.exceptions.DBusException('Foo running', name='com.ubuntu.Upstart0_6.Error.AlreadyStarted')",
            NULL);

        dbus_test_dbus_mock_object_add_method(mock, jobobj, "Stop", G_VARIANT_TYPE("(asb)"), NULL, "", NULL);

        dbus_test_dbus_mock_object_add_method(mock, jobobj, "GetAllInstances", NULL, G_VARIANT_TYPE("ao"),
                                              "ret = [ dbus.ObjectPath('/com/test/app_instance') ]", NULL);

        DbusTestDbusMockObject* instobj =
            dbus_test_dbus_mock_get_object(mock, "/com/test/app_instance", "com.ubuntu.Upstart0_6.Instance", NULL);
        dbus_test_dbus_mock_object_add_property(mock, instobj, "name", G_VARIANT_TYPE_STRING,
                                                g_variant_new_string("com.test.good_application_1.2.3"), NULL);
        gchar* process_var = g_strdup_printf("[('main', %d)]", getpid());
        dbus_test_dbus_mock_object_add_property(mock, instobj, "processes", G_VARIANT_TYPE("a(si)"),
                                                g_variant_new_parsed(process_var), NULL);
        g_free(process_var);

        /*  Legacy App */
        DbusTestDbusMockObject* ljobobj =
            dbus_test_dbus_mock_get_object(mock, "/com/test/application_legacy", "com.ubuntu.Upstart0_6.Job", NULL);

        dbus_test_dbus_mock_object_add_method(mock, ljobobj, "Start", G_VARIANT_TYPE("(asb)"), NULL, "", NULL);

        dbus_test_dbus_mock_object_add_method(mock, ljobobj, "Stop", G_VARIANT_TYPE("(asb)"), NULL, "", NULL);

        dbus_test_dbus_mock_object_add_method(mock, ljobobj, "GetAllInstances", NULL, G_VARIANT_TYPE("ao"),
                                              "ret = [ dbus.ObjectPath('/com/test/legacy_app_instance') ]", NULL);

        DbusTestDbusMockObject* linstobj = dbus_test_dbus_mock_get_object(mock, "/com/test/legacy_app_instance",
                                                                          "com.ubuntu.Upstart0_6.Instance", NULL);
        dbus_test_dbus_mock_object_add_property(mock, linstobj, "name", G_VARIANT_TYPE_STRING,
                                                g_variant_new_string("multiple-2342345"), NULL);
        dbus_test_dbus_mock_object_add_property(mock, linstobj, "processes", G_VARIANT_TYPE("a(si)"),
                                                g_variant_new_parsed("[('main', 5678)]"), NULL);

        /*  Untrusted Helper */
        DbusTestDbusMockObject* uhelperobj =
            dbus_test_dbus_mock_get_object(mock, "/com/test/untrusted/helper", "com.ubuntu.Upstart0_6.Job", NULL);

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
                            "if (exec_app != \"\"):\n"
                            "    proc = subprocess.Popen(exec_app, shell=True, stdout=subprocess.PIPE)\n"
                            "target.close\n"
                            , NULL);



        dbus_test_dbus_mock_object_add_method(mock, uhelperobj, "Stop", G_VARIANT_TYPE("(asb)"), NULL, "", NULL);

        dbus_test_dbus_mock_object_add_method(mock, uhelperobj, "GetAllInstances", NULL, G_VARIANT_TYPE("ao"),
                                              "ret = [ dbus.ObjectPath('/com/test/untrusted/helper/instance'), "
                                              "dbus.ObjectPath('/com/test/untrusted/helper/multi_instance') ]",
                                              NULL);

        DbusTestDbusMockObject* uhelperinstance = dbus_test_dbus_mock_get_object(
            mock, "/com/test/untrusted/helper/instance", "com.ubuntu.Upstart0_6.Instance", NULL);
        dbus_test_dbus_mock_object_add_property(mock, uhelperinstance, "name", G_VARIANT_TYPE_STRING,
                                                g_variant_new_string("untrusted-type::com.foo_bar_43.23.12"), NULL);

        DbusTestDbusMockObject* unhelpermulti = dbus_test_dbus_mock_get_object(
            mock, "/com/test/untrusted/helper/multi_instance", "com.ubuntu.Upstart0_6.Instance", NULL);
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

    virtual void TearDown()
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
        ASSERT_EQ(nullptr, bus);
    }

    GVariant* find_env(GVariant* env_array, const gchar* var)
    {
        unsigned int i;
        GVariant* retval = nullptr;

        for (i = 0; i < g_variant_n_children(env_array); i++)
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

    bool check_env(GVariant* env_array, const gchar* var, const gchar* value)
    {
        bool found = false;
        GVariant* val = find_env(env_array, var);
        if (val == nullptr)
        {
            return false;
        }

        const gchar* envvar = g_variant_get_string(val, nullptr);

        gchar* combined = g_strdup_printf("%s=%s", var, value);
        if (g_strcmp0(envvar, combined) == 0)
        {
            found = true;
        }

        g_variant_unref(val);

        return found;
    }

    void pause(guint time = 0)
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
};

TEST_F(LibUAL, StartHelper)
{
    DbusTestDbusMockObject* obj =
        dbus_test_dbus_mock_get_object(mock, "/com/test/untrusted/helper", "com.ubuntu.Upstart0_6.Job", NULL);

    BackupHelper helper("com.test.multiple_first_1.2.3");

    QSignalSpy spy(&helper, &BackupHelper::started);

    helper.start(1999);

    guint len = 0;
    auto calls = dbus_test_dbus_mock_object_get_method_calls(mock, obj, "Start", &len, NULL);
    EXPECT_NE(nullptr, calls);
    EXPECT_EQ(1, len);

    auto env = g_variant_get_child_value(calls->params, 0);
    EXPECT_TRUE(check_env(env, "APP_ID", "com.test.multiple_first_1.2.3"));
    EXPECT_TRUE(
        check_env(env, "APP_URIS", "'/custom/click/dekko.dekkoproject/0.6.20/backup-helper'"));
    EXPECT_TRUE(check_env(env, "HELPER_TYPE", "backup-helper"));
    EXPECT_FALSE(check_env(env, "INSTANCE_ID", NULL));
    g_variant_unref(env);

    DbusTestDbusMockObject* objUpstart =
        dbus_test_dbus_mock_get_object(mock, "/com/ubuntu/Upstart", "com.ubuntu.Upstart0_6", NULL);

    /* Basic start */
    dbus_test_dbus_mock_object_emit_signal(
        mock, objUpstart, "EventEmitted", G_VARIANT_TYPE("(sas)"),
        g_variant_new_parsed("('started', ['JOB=untrusted-helper', 'INSTANCE=backup-helper::com.test.multiple_first_1.2.3'])"),
        NULL);

    // we set a timeout of 5 seconds waiting for the signal to be emitted,
    // which should never be reached
    ASSERT_TRUE(spy.wait(5000));

    // check that we've got exactly one signal
    ASSERT_EQ(spy.count(), 1);

    g_usleep(100000);
    while (g_main_pending())
    {
        g_main_iteration(TRUE);
    }

    helper.stop();
    return;
}

TEST_F(LibUAL, StopHelper)
{
    DbusTestDbusMockObject* obj =
        dbus_test_dbus_mock_get_object(mock, "/com/test/untrusted/helper", "com.ubuntu.Upstart0_6.Job", NULL);

    BackupHelper helper("com.bar_foo_8432.13.1");
    QSignalSpy spy(&helper, &BackupHelper::finished);

    helper.stop();
    ASSERT_EQ(dbus_test_dbus_mock_object_check_method_call(mock, obj, "Stop", NULL, NULL), 1);

    guint len = 0;
    auto calls = dbus_test_dbus_mock_object_get_method_calls(mock, obj, "Stop", &len, NULL);
    EXPECT_NE(nullptr, calls);
    EXPECT_EQ(1, len);

    EXPECT_STREQ("Stop", calls->name);
    EXPECT_EQ(2, g_variant_n_children(calls->params));

    auto block = g_variant_get_child_value(calls->params, 1);
    EXPECT_TRUE(g_variant_get_boolean(block));
    g_variant_unref(block);

    auto env = g_variant_get_child_value(calls->params, 0);
    EXPECT_TRUE(check_env(env, "APP_ID", "com.bar_foo_8432.13.1"));
    EXPECT_TRUE(check_env(env, "HELPER_TYPE", "backup-helper"));
    EXPECT_TRUE(check_env(env, "INSTANCE_ID", "24034582324132"));
    g_variant_unref(env);

    ASSERT_TRUE(dbus_test_dbus_mock_object_clear_method_calls(mock, obj, NULL));

    DbusTestDbusMockObject* objUpstart =
        dbus_test_dbus_mock_get_object(mock, "/com/ubuntu/Upstart", "com.ubuntu.Upstart0_6", NULL);

    dbus_test_dbus_mock_object_emit_signal(
        mock, objUpstart, "EventEmitted", G_VARIANT_TYPE("(sas)"),
        g_variant_new_parsed(
            "('stopped', ['JOB=untrusted-helper', 'INSTANCE=backup-helper::com.bar_foo_8432.13.1'])"),
        NULL);

    // we set a timeout of 5 seconds waiting for the signal to be emitted,
    // which should never be reached
    ASSERT_TRUE(spy.wait(5000));

    // check that we've got exactly one signal
    ASSERT_EQ(spy.count(), 1);

    g_usleep(100000);
    while (g_main_pending())
    {
        g_main_iteration(TRUE);
    }

//    ASSERT_EQ(stop_data.count, 1);

    return;
}

typedef struct
{
    unsigned int count;
    const gchar* appid;
    const gchar* type;
    const gchar* instance;
} helper_observer_data_t;

static void helper_observer_cb(const gchar* appid, const gchar* instance, const gchar* type, gpointer user_data)
{
    helper_observer_data_t* data = (helper_observer_data_t*)user_data;

    if (g_strcmp0(data->appid, appid) == 0 && g_strcmp0(data->type, type) == 0 &&
        g_strcmp0(data->instance, instance) == 0)
    {
        data->count++;
    }
}

TEST_F(LibUAL, StartStopHelperObserver)
{
    helper_observer_data_t start_data = {
        .count = 0, .appid = "com.foo_foo_1.2.3", .type = "my-type-is-scorpio", .instance = nullptr};
    helper_observer_data_t stop_data = {
        .count = 0, .appid = "com.bar_bar_44.32", .type = "my-type-is-libra", .instance = "1234"};

    ASSERT_TRUE(ubuntu_app_launch_observer_add_helper_started(helper_observer_cb, "my-type-is-scorpio", &start_data));
    ASSERT_TRUE(ubuntu_app_launch_observer_add_helper_stop(helper_observer_cb, "my-type-is-libra", &stop_data));

    DbusTestDbusMockObject* obj =
        dbus_test_dbus_mock_get_object(mock, "/com/ubuntu/Upstart", "com.ubuntu.Upstart0_6", NULL);

    /* Basic start */
    dbus_test_dbus_mock_object_emit_signal(
        mock, obj, "EventEmitted", G_VARIANT_TYPE("(sas)"),
        g_variant_new_parsed("('started', ['JOB=untrusted-helper', 'INSTANCE=my-type-is-scorpio::com.foo_foo_1.2.3'])"),
        NULL);

    g_usleep(100000);
    while (g_main_pending())
    {
        g_main_iteration(TRUE);
    }

    ASSERT_EQ(start_data.count, 1);

    /* Basic stop */
    dbus_test_dbus_mock_object_emit_signal(
        mock, obj, "EventEmitted", G_VARIANT_TYPE("(sas)"),
        g_variant_new_parsed(
            "('stopped', ['JOB=untrusted-helper', 'INSTANCE=my-type-is-libra:1234:com.bar_bar_44.32'])"),
        NULL);

    g_usleep(100000);
    while (g_main_pending())
    {
        g_main_iteration(TRUE);
    }

    ASSERT_EQ(stop_data.count, 1);


    /* Remove */
    ASSERT_TRUE(
        ubuntu_app_launch_observer_delete_helper_started(helper_observer_cb, "my-type-is-scorpio", &start_data));
    ASSERT_TRUE(ubuntu_app_launch_observer_delete_helper_stop(helper_observer_cb, "my-type-is-libra", &stop_data));
}

int main(int argc, char** argv)
{
    QCoreApplication qt_app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
