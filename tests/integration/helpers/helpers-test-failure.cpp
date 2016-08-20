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

TEST_F(TestHelpers, BackupHelperWritesTooLittle)
{
    XdgUserDirsSandbox tmp_dir;

    // starts the services, including keeper-service
    startTasks();

    // ask for a list of backup choices
    QDBusReply<QVariantDictMap> choices = user_iface->call("GetBackupChoices");
    EXPECT_TRUE(choices.isValid()) << qPrintable(choices.error().message());

    auto user_option = QStringLiteral("XDG_MUSIC_DIR");

    auto user_dir = qgetenv(user_option.toLatin1().data());
    ASSERT_FALSE(user_dir.isEmpty());
    qDebug() << "USER DIR:" << user_dir;

    // fill something in the music dir
    FileUtils::fillTemporaryDirectory(user_dir, qrand() % 1000);

    // search for the user folder uuid
    auto user_folder_uuid = getUUIDforXdgFolderPath(user_dir, choices.value());
    ASSERT_FALSE(user_folder_uuid.isEmpty());
    qDebug() << "User folder UUID is:" << user_folder_uuid;

    // Now we know the music folder uuid, let's start the backup for it.
    QDBusReply<void> backup_reply = user_iface->call("StartBackup", QStringList{user_folder_uuid});
    ASSERT_TRUE(backup_reply.isValid()) << qPrintable(QDBusConnection::sessionBus().lastError().message());

    EXPECT_TRUE(wait_for_tasks_to_finish());

    // check that the this task failed
    QVariantDictMap state = user_iface->state();
    auto iter = state.find(user_folder_uuid);
    EXPECT_TRUE(iter != state.end());
    auto state_values = state[user_folder_uuid];

    EXPECT_EQ(std::string{"failed"}, state_values["action"].toString().toStdString());
    EXPECT_EQ(std::string{"Music"}, state_values["display-name"].toString().toStdString());
    // sent 1 byte less than the expected, so percentage has to be less than 1.0
    EXPECT_LT(state_values["percent-done"].toFloat(), 1.0f);

    // let's leave things clean
    EXPECT_TRUE(removeHelperMarkBeforeStarting());
}
