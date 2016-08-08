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
};

TEST_F(TestHelpers, StartFullTest)
{
    g_setenv("KEEPER_TEST_HELPER", TEST_SIMPLE_HELPER_FAILURE, TRUE);

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
    qDebug() << "USER DIR: " << user_dir;

    // fill something in the music dir
    ASSERT_TRUE(FileUtils::fillTemporaryDirectory(user_dir, qrand() % 1000));

    // search for the user folder uuid
    auto user_folder_uuid = getUUIDforXdgFolderPath(user_dir, choices.value());
    ASSERT_FALSE(user_folder_uuid.isEmpty());
    qDebug() << "User folder UUID is: " << user_folder_uuid;

    // Now we know the music folder uuid, let's start the backup for it.
    QDBusReply<void> backup_reply = user_iface->call("StartBackup", QStringList{user_folder_uuid});
    ASSERT_TRUE(backup_reply.isValid()) << qPrintable(QDBusConnection::sessionBus().lastError().message());

    // Wait until the helper finishes
    EXPECT_TRUE(waitUntilHelperFinishes(DEKKO_APP_ID, 15000, 1));

    // check that the content of the file is the expected
    EXPECT_EQ(checkStorageFrameworkNbFiles(), 0);
    // let's leave things clean
    EXPECT_TRUE(removeHelperMarkBeforeStarting());
}
