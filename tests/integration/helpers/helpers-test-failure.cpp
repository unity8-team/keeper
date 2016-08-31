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

TEST_F(TestHelpers, BackupHelperWritesTooMuch)
{
    XdgUserDirsSandbox tmp_dir;

    // starts the services, including keeper-service
    start_tasks();

    QDBusConnection connection = QDBusConnection::sessionBus();
    QSharedPointer<DBusInterfaceKeeperUser> user_iface(new DBusInterfaceKeeperUser(
                                                            DBusTypes::KEEPER_SERVICE,
                                                            DBusTypes::KEEPER_USER_PATH,
                                                            connection
                                                        ) );

    ASSERT_TRUE(user_iface->isValid()) << qPrintable(QDBusConnection::sessionBus().lastError().message());

    // ask for a list of backup choices
    QDBusReply<QVariantDictMap> choices = user_iface->call("GetBackupChoices");
    EXPECT_TRUE(choices.isValid()) << qPrintable(choices.error().message());

    auto user_option = QStringLiteral("XDG_MUSIC_DIR");

    auto user_dir = qgetenv(user_option.toLatin1().data());
    ASSERT_FALSE(user_dir.isEmpty());
    qDebug() << "USER DIR:" << user_dir;

    // fill something in the music dir
    ASSERT_TRUE(FileUtils::fillTemporaryDirectory(user_dir, qrand() % 1000));

    // search for the user folder uuid
    auto user_folder_uuid = get_uuid_for_xdg_folder_path(user_dir, choices.value());
    ASSERT_FALSE(user_folder_uuid.isEmpty());
    qDebug() << "User folder UUID is:" << user_folder_uuid;

    QFile helper_mark(SIMPLE_HELPER_MARK_FILE_PATH);
    qDebug() << "Helper mark exists before calling StartBackup..." << helper_mark.exists();

    // Now we know the music folder uuid, let's start the backup for it.
    QDBusReply<void> backup_reply = user_iface->call("StartBackup", QStringList{user_folder_uuid});
    ASSERT_TRUE(backup_reply.isValid()) << qPrintable(QDBusConnection::sessionBus().lastError().message());

    // wait until all the tasks have the action state "failed"
    EXPECT_TRUE(wait_for_all_tasks_have_action_state({user_folder_uuid}, "failed", user_iface));

    // check that the content of the file is the expected
    EXPECT_EQ(0, check_storage_framework_nb_files());

    // check that the state is failed
    QVariantDictMap state = user_iface->state();

    // check that the state has the uuid
    QVariantDictMap::const_iterator iter = state.find(user_folder_uuid);
    EXPECT_TRUE(iter != state.end());
    auto state_values = state[user_folder_uuid];

    EXPECT_EQ(std::string{"failed"}, state_values["action"].toString().toStdString());
    EXPECT_EQ(std::string{"Music"}, state_values["display-name"].toString().toStdString());
    // sent 1 byte more than the expected, so percentage has to be greater than 1.0
    EXPECT_GT(state_values["percent-done"].toFloat(), 1.0);
}
