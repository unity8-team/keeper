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

class TestHelpers: public TestHelpersBase
{
    using super = TestHelpersBase;

    void SetUp() override
    {
        super::SetUp();
        init_helper_registry(HELPER_REGISTRY);
    }
};


TEST_F(TestHelpers, StartHelper)
{
    // starts the services, including keeper-service
    startTasks();

    DbusTestDbusMockObject* obj =
        dbus_test_dbus_mock_get_object(mock, UNTRUSTED_HELPER_PATH, UPSTART_JOB, NULL);

    BackupHelper helper("com.test.multiple_first_1.2.3");

    QSignalSpy spy(&helper, &BackupHelper::state_changed);

    QStringList urls;
    urls << DEKKO_HELPER_BIN << DEKKO_HELPER_DIR;
    helper.start(urls);

    guint len = 0;
    auto calls = dbus_test_dbus_mock_object_get_method_calls(mock, obj, "Start", &len, NULL);
    EXPECT_NE(nullptr, calls);
    EXPECT_EQ(1, len);

    auto env = g_variant_get_child_value(calls->params, 0);
    EXPECT_ENV("com.test.multiple_first_1.2.3", env, "APP_ID");

    QString appUrisStr = QString("'%1' '%2'").arg(DEKKO_HELPER_BIN).arg(DEKKO_HELPER_DIR);
    EXPECT_ENV(appUrisStr.toLocal8Bit().data(), env, "APP_URIS");
    EXPECT_ENV("backup-helper", env, "HELPER_TYPE");
    EXPECT_TRUE(have_env(env, "INSTANCE_ID"));
    g_variant_unref(env);

    DbusTestDbusMockObject* objUpstart =
        dbus_test_dbus_mock_get_object(mock, UPSTART_PATH, UPSTART_INTERFACE, NULL);

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
}

TEST_F(TestHelpers, StopHelper)
{
    // starts the services, including keeper-service
    startTasks();

    DbusTestDbusMockObject* obj =
        dbus_test_dbus_mock_get_object(mock, UNTRUSTED_HELPER_PATH, UPSTART_JOB, NULL);

    BackupHelper helper("com.bar_foo_8432.13.1");
    QSignalSpy spy(&helper, &BackupHelper::state_changed);

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
    EXPECT_ENV("com.bar_foo_8432.13.1", env, "APP_ID");
    EXPECT_ENV("backup-helper", env, "HELPER_TYPE");
    EXPECT_ENV("24034582324132", env, "INSTANCE_ID");
    g_variant_unref(env);

    ASSERT_TRUE(dbus_test_dbus_mock_object_clear_method_calls(mock, obj, NULL));

    DbusTestDbusMockObject* objUpstart =
        dbus_test_dbus_mock_get_object(mock, UPSTART_PATH, UPSTART_INTERFACE, NULL);

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

TEST_F(TestHelpers, StartStopHelperObserver)
{
    // starts the services, including keeper-service
    startTasks();

    helper_observer_data_t start_data = {
        .count = 0, .appid = "com.foo_foo_1.2.3", .type = "my-type-is-scorpio", .instance = nullptr};
    helper_observer_data_t stop_data = {
        .count = 0, .appid = "com.bar_bar_44.32", .type = "my-type-is-libra", .instance = "1234"};

    ASSERT_TRUE(ubuntu_app_launch_observer_add_helper_started(helper_observer_cb, "my-type-is-scorpio", &start_data));
    ASSERT_TRUE(ubuntu_app_launch_observer_add_helper_stop(helper_observer_cb, "my-type-is-libra", &stop_data));

    DbusTestDbusMockObject* obj =
        dbus_test_dbus_mock_get_object(mock, UPSTART_PATH, UPSTART_INTERFACE, NULL);

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

TEST_F(TestHelpers, StartFullTest)
{
    XdgUserDirsSandbox tmp_dir;

    // starts the services, including keeper-service
    startTasks();

    QDBusConnection connection = QDBusConnection::sessionBus();
    QScopedPointer<DBusInterfaceKeeperUser> user_iface(new DBusInterfaceKeeperUser(
                                                            DBusTypes::KEEPER_SERVICE,
                                                            DBusTypes::KEEPER_USER_PATH,
                                                            connection
                                                        ) );

    ASSERT_TRUE(user_iface->isValid()) << qPrintable(QDBusConnection::sessionBus().lastError().message());

    // ask for a list of backup choices
    QDBusReply<QVariantDictMap> choices = user_iface->call("GetBackupChoices");
    EXPECT_TRUE(choices.isValid()) << qPrintable(choices.error().message());

    QString user_option = "XDG_MUSIC_DIR";

    auto user_dir = qgetenv(user_option.toLatin1().data());
    ASSERT_FALSE(user_dir.isEmpty());
    qDebug() << "USER DIR:" << user_dir;

    // fill something in the music dir
    ASSERT_TRUE(FileUtils::fillTemporaryDirectory(user_dir, qrand() % 1000));

    // search for the user folder uuid
    auto user_folder_uuid = getUUIDforXdgFolderPath(user_dir, choices.value());
    ASSERT_FALSE(user_folder_uuid.isEmpty());
    qDebug() << "User folder UUID is:" << user_folder_uuid;

    QString user_option_2 = "XDG_VIDEOS_DIR";

    auto user_dir_2 = qgetenv(user_option_2.toLatin1().data());
    ASSERT_FALSE(user_dir_2.isEmpty());
    qDebug() << "USER DIR 2:" << user_dir_2;

    // fill something in the music dir
    ASSERT_TRUE(FileUtils::fillTemporaryDirectory(user_dir_2, qrand() % 1000));

    // search for the user folder uuid
    auto user_folder_uuid_2 = getUUIDforXdgFolderPath(user_dir_2, choices.value());
    ASSERT_FALSE(user_folder_uuid_2.isEmpty());
    qDebug() << "User folder 2 UUID is:" << user_folder_uuid_2;

    // Now we know the music folder uuid, let's start the backup for it.
    QDBusReply<void> backup_reply = user_iface->call("StartBackup", QStringList{user_folder_uuid, user_folder_uuid_2});
    ASSERT_TRUE(backup_reply.isValid()) << qPrintable(QDBusConnection::sessionBus().lastError().message());

    // Wait until the helper finishes
    EXPECT_TRUE(waitUntilHelperFinishes(DEKKO_APP_ID, 15000, 2));

    // check that the content of the file is the expected
    EXPECT_TRUE(checkStorageFrameworkFiles(QStringList{user_dir, user_dir_2}));
    // let's leave things clean
    EXPECT_TRUE(removeHelperMarkBeforeStarting());
}

TEST_F(TestHelpers, Inactivity)
{
    // starts the services, including keeper-service
    startTasks();

    DbusTestDbusMockObject* obj =
        dbus_test_dbus_mock_get_object(mock, UNTRUSTED_HELPER_PATH, UPSTART_JOB, NULL);

    BackupHelper helper("com.bar_foo_8432.13.1");

    QStringList urls;
    urls << DEKKO_HELPER_BIN << DEKKO_HELPER_DIR;
    helper.start(urls);

    DbusTestDbusMockObject* objUpstart =
        dbus_test_dbus_mock_get_object(mock, UPSTART_PATH, UPSTART_INTERFACE, NULL);

    /* Basic start */
    dbus_test_dbus_mock_object_emit_signal(
        mock, objUpstart, "EventEmitted", G_VARIANT_TYPE("(sas)"),
        g_variant_new_parsed("('started', ['JOB=untrusted-helper', 'INSTANCE=backup-helper::com.bar_foo_8432.13.1'])"),
        NULL);

    QElapsedTimer timer;
    timer.start();
    int nb_stop_calls = 0;
    // we wait 1 second more compared to the inactivity time...
    while(!timer.hasExpired(BackupHelper::MAX_INACTIVITY_TIME + 1000) && (nb_stop_calls == 0))
    {
        nb_stop_calls = dbus_test_dbus_mock_object_check_method_call(mock, obj, "Stop", NULL, NULL);
        QCoreApplication::processEvents();
    }

    EXPECT_EQ(nb_stop_calls, 1);

    dbus_test_dbus_mock_object_emit_signal(
            mock, objUpstart, "EventEmitted", G_VARIANT_TYPE("(sas)"),
            g_variant_new_parsed(
                "('stopped', ['JOB=untrusted-helper', 'INSTANCE=backup-helper::com.bar_foo_8432.13.1'])"),
            NULL);

    g_usleep(100000);
    while (g_main_pending())
    {
        g_main_iteration(TRUE);
    }
}
