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
#include "tests/utils/storage-framework-local.h"

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
    start_tasks();

    BackupHelper helper("com.test.multiple_first_1.2.3");

    QSignalSpy spy(&helper, &BackupHelper::state_changed);

    helper.start({"/bin/ls","/tmp"});

    // wait for 2 signals.
    // One for started, another one when the helper stops
    WAIT_FOR_SIGNALS(spy, 2, 15000);

    ASSERT_EQ(spy.count(), 2);
    QList<QVariant> arguments = spy.takeFirst();
    EXPECT_EQ(qvariant_cast<Helper::State>(arguments.at(0)), Helper::State::STARTED);
    arguments = spy.takeFirst();
    EXPECT_EQ(qvariant_cast<Helper::State>(arguments.at(0)), Helper::State::COMPLETE);
}

TEST_F(TestHelpers, StartFullTest)
{
    XdgUserDirsSandbox tmp_dir;

    // starts the services, including keeper-service
    start_tasks();

    QSharedPointer<DBusInterfaceKeeperUser> user_iface(new DBusInterfaceKeeperUser(
                                                            DBusTypes::KEEPER_SERVICE,
                                                            DBusTypes::KEEPER_USER_PATH,
                                                            dbus_test_runner.sessionConnection()
                                                        ) );

    ASSERT_TRUE(user_iface->isValid()) << qPrintable(dbus_test_runner.sessionConnection().lastError().message());

    // ask for a list of backup choices
    QDBusReply<QVariantDictMap> choices = user_iface->call("GetBackupChoices");
    EXPECT_TRUE(choices.isValid()) << qPrintable(choices.error().message());

    QString user_option = QStringLiteral("XDG_MUSIC_DIR");

    auto user_dir = qgetenv(user_option.toLatin1().data());
    ASSERT_FALSE(user_dir.isEmpty());
    qDebug() << "USER DIR:" << user_dir;

    // fill something in the music dir
    FileUtils::fillTemporaryDirectory(user_dir, qrand() % 100);

    // search for the user folder uuid
    auto user_folder_uuid = get_uuid_for_xdg_folder_path(user_dir, choices.value());
    ASSERT_FALSE(user_folder_uuid.isEmpty());
    qDebug() << "User folder UUID is:" << user_folder_uuid;

    QString user_option_2 = QStringLiteral("XDG_VIDEOS_DIR");

    auto user_dir_2 = qgetenv(user_option_2.toLatin1().data());
    ASSERT_FALSE(user_dir_2.isEmpty());
    qDebug() << "USER DIR 2:" << user_dir_2;

    // fill something in the music dir
    FileUtils::fillTemporaryDirectory(user_dir_2, qrand() % 100);

    // search for the user folder uuid
    auto user_folder_uuid_2 = get_uuid_for_xdg_folder_path(user_dir_2, choices.value());
    ASSERT_FALSE(user_folder_uuid_2.isEmpty());
    qDebug() << "User folder 2 UUID is:" << user_folder_uuid_2;

    QSharedPointer<DBusPropertiesInterface> properties_interface(new DBusPropertiesInterface(
                                                            DBusTypes::KEEPER_SERVICE,
                                                            DBusTypes::KEEPER_USER_PATH,
                                                            dbus_test_runner.sessionConnection()
                                                        ) );

    ASSERT_TRUE(properties_interface->isValid()) << qPrintable(QDBusConnection::sessionBus().lastError().message());

    QSignalSpy spy(properties_interface.data(),&DBusPropertiesInterface::PropertiesChanged);

    // Now we know the music folder uuid, let's start the backup for it.
    QDBusReply<void> backup_reply = user_iface->call("StartBackup", QStringList{user_folder_uuid, user_folder_uuid_2});
    ASSERT_TRUE(backup_reply.isValid()) << qPrintable(dbus_test_runner.sessionConnection().lastError().message());

    // waits until all tasks are complete, recording PropertiesChanged signals
    // and checks all the recorded values
    EXPECT_TRUE(capture_and_check_state_until_all_tasks_complete(spy, {user_folder_uuid, user_folder_uuid_2}, "complete"));

    // wait until all the tasks have the action state "complete"
    // this one uses pooling so it should just call Get once
    EXPECT_TRUE(wait_for_all_tasks_have_action_state({user_folder_uuid, user_folder_uuid_2}, "complete", user_iface));

    // check that the content of the file is the expected
    EXPECT_TRUE(StorageFrameworkLocalUtils::check_storage_framework_files(QStringList{user_dir, user_dir_2}));
}

TEST_F(TestHelpers, StartFullTestCancelling)
{
    XdgUserDirsSandbox tmp_dir;

    // starts the services, including keeper-service
    start_tasks();

    QSharedPointer<DBusInterfaceKeeperUser> user_iface(new DBusInterfaceKeeperUser(
                                                            DBusTypes::KEEPER_SERVICE,
                                                            DBusTypes::KEEPER_USER_PATH,
                                                            dbus_test_runner.sessionConnection()
                                                        ) );

    ASSERT_TRUE(user_iface->isValid()) << qPrintable(dbus_test_runner.sessionConnection().lastError().message());

    // ask for a list of backup choices
    QDBusReply<QVariantDictMap> choices = user_iface->call("GetBackupChoices");
    EXPECT_TRUE(choices.isValid()) << qPrintable(choices.error().message());

    QString user_option = QStringLiteral("XDG_MUSIC_DIR");

    auto user_dir = qgetenv(user_option.toLatin1().data());
    ASSERT_FALSE(user_dir.isEmpty());
    qDebug() << "USER DIR:" << user_dir;

    // fill something in the music dir
    FileUtils::fillTemporaryDirectory(user_dir, qrand() % 100);

    // search for the user folder uuid
    auto user_folder_uuid = get_uuid_for_xdg_folder_path(user_dir, choices.value());
    ASSERT_FALSE(user_folder_uuid.isEmpty());
    qDebug() << "User folder UUID is:" << user_folder_uuid;

    QString user_option_2 = QStringLiteral("XDG_VIDEOS_DIR");

    auto user_dir_2 = qgetenv(user_option_2.toLatin1().data());
    ASSERT_FALSE(user_dir_2.isEmpty());
    qDebug() << "USER DIR 2:" << user_dir_2;

    // fill something in the music dir
    FileUtils::fillTemporaryDirectory(user_dir_2, qrand() % 100);

    // search for the user folder uuid
    auto user_folder_uuid_2 = get_uuid_for_xdg_folder_path(user_dir_2, choices.value());
    ASSERT_FALSE(user_folder_uuid_2.isEmpty());
    qDebug() << "User folder 2 UUID is:" << user_folder_uuid_2;

    QSharedPointer<DBusPropertiesInterface> properties_interface(new DBusPropertiesInterface(
                                                            DBusTypes::KEEPER_SERVICE,
                                                            DBusTypes::KEEPER_USER_PATH,
                                                            dbus_test_runner.sessionConnection()
                                                        ) );

    ASSERT_TRUE(properties_interface->isValid()) << qPrintable(QDBusConnection::sessionBus().lastError().message());

    QSignalSpy spy(properties_interface.data(),&DBusPropertiesInterface::PropertiesChanged);

    // Now we know the music folder uuid, let's start the backup for it.
    QDBusReply<void> backup_reply = user_iface->call("StartBackup", QStringList{user_folder_uuid, user_folder_uuid_2});
    ASSERT_TRUE(backup_reply.isValid()) << qPrintable(dbus_test_runner.sessionConnection().lastError().message());

    EXPECT_TRUE(cancel_first_task_at_percentage(spy, 0.20, user_iface));

    // wait until all the tasks have the action state "complete"
    // this one uses pooling so it should just call Get once
    EXPECT_TRUE(wait_for_all_tasks_have_action_state({user_folder_uuid, user_folder_uuid_2}, "cancelled", user_iface));

    // check that we have no files in storage framework
    EXPECT_EQ(0, StorageFrameworkLocalUtils::check_storage_framework_nb_files());
}

TEST_F(TestHelpers, CheckBadUUIDS)
{
    XdgUserDirsSandbox tmp_dir;

    // starts the services, including keeper-service
    start_tasks();

    QSharedPointer<DBusInterfaceKeeperUser> user_iface(new DBusInterfaceKeeperUser(
                                                            DBusTypes::KEEPER_SERVICE,
                                                            DBusTypes::KEEPER_USER_PATH,
                                                            dbus_test_runner.sessionConnection()
                                                        ) );

    ASSERT_TRUE(user_iface->isValid()) << qPrintable(dbus_test_runner.sessionConnection().lastError().message());

    // Now we know the music folder uuid, let's start the backup for it.
    QDBusReply<void> backup_reply = user_iface->call("StartBackup", QStringList{"bad_uuid_1", "bad_uuid_2"});

    ASSERT_FALSE(backup_reply.isValid());

    // check the error message
    // the uuids are not printed always in the same order
    QString error_message = dbus_test_runner.sessionConnection().lastError().message();
    EXPECT_TRUE(error_message.startsWith("unhandled uuids: "));
    EXPECT_TRUE(error_message.contains("bad_uuid_1"));
    EXPECT_TRUE(error_message.contains("bad_uuid_2"));
}

TEST_F(TestHelpers, SimplyCheckThatTheSecondDBusInterfaceIsFine)
{
    XdgUserDirsSandbox tmp_dir;

    // starts the services, including keeper-service
    start_tasks();

    QSharedPointer<DBusInterfaceKeeperUser> user_iface(new DBusInterfaceKeeperUser(
                                                            DBusTypes::KEEPER_SERVICE,
                                                            DBusTypes::KEEPER_USER_PATH,
                                                            dbus_test_runner.sessionConnection()
                                                        ) );

    ASSERT_TRUE(user_iface->isValid()) << qPrintable(dbus_test_runner.sessionConnection().lastError().message());

}

TEST_F(TestHelpers, BadHelperPath)
{
    // starts the services, including keeper-service
    start_tasks();

    BackupHelper helper("com.bar_foo_8432.13.1");

    QSignalSpy spy(&helper, &BackupHelper::state_changed);
    QStringList urls;
    urls << "blah" << "/tmp";
    helper.start(urls);

    WAIT_FOR_SIGNALS(spy, 1, Helper::MAX_UAL_WAIT_TIME + 1000);

    ASSERT_EQ(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    EXPECT_EQ(qvariant_cast<Helper::State>(arguments.at(0)), Helper::State::FAILED);
}

TEST_F(TestHelpers, Inactivity)
{
    // starts the services, including keeper-service
    start_tasks();

    BackupHelper helper("com.bar_foo_8432.13.1");

    QSignalSpy spy(&helper, &BackupHelper::state_changed);
    QStringList urls;
    urls << TEST_INACTIVE_HELPER << "/tmp";
    helper.start(urls);

    // wait 15 seconds at most.
    // the inactive helper sleeps for 100 seconds so
    // if we get the 2 signals it means it was stopped due to inactivity
    // We can also check at the end for the state, which should be CANCELLED
    WAIT_FOR_SIGNALS(spy, 2, BackupHelper::MAX_INACTIVITY_TIME + 2000);

    ASSERT_EQ(spy.count(), 2);
    QList<QVariant> arguments = spy.takeFirst();
    EXPECT_EQ(qvariant_cast<Helper::State>(arguments.at(0)), Helper::State::STARTED);
    arguments = spy.takeFirst();
    EXPECT_EQ(qvariant_cast<Helper::State>(arguments.at(0)), Helper::State::CANCELLED);
}
