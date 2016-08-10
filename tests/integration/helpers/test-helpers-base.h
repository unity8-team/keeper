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
#pragma once

#include <future>
#include <thread>

#include <QCoreApplication>
#include <QSignalSpy>

#include <gtest/gtest.h>
#include <gio/gio.h>
#include <ubuntu-app-launch/registry.h>
#include <libdbustest/dbus-test.h>

#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbustest/QProcessDBusService.h>
#include <libqtdbusmock/DBusMock.h>

#include "mir-mock.h"
#include <helper/backup-helper.h>
#include <qdbus-stubs/dbus-types.h>
#include "qdbus-stubs/keeper_user_interface.h"

#include "tests/fakes/fake-backup-helper.h"
#include "tests/utils/file-utils.h"
#include "tests/utils/xdg-user-dirs-sandbox.h"
#include "../../../src/service/app-const.h"


namespace
{
constexpr char const UPSTART_PATH[] = "/com/ubuntu/Upstart";
constexpr char const UPSTART_INTERFACE[] = "com.ubuntu.Upstart0_6";
constexpr char const UPSTART_INSTANCE[] = "com.ubuntu.Upstart0_6.Instance";
constexpr char const UPSTART_JOB[] = "com.ubuntu.Upstart0_6.Job";
constexpr char const UNTRUSTED_HELPER_PATH[] = "/com/test/untrusted/helper";
}

extern "C" {
    #include <ubuntu-app-launch.h>
    #include <fcntl.h>
}

class TestHelpersBase : public ::testing::Test
{
public:
    TestHelpersBase() = default;

    ~TestHelpersBase() = default;

protected:
    DbusTestService* service = nullptr;
    DbusTestDbusMock* mock = nullptr;
    DbusTestDbusMock* cgmock = nullptr;
    GDBusConnection* bus = nullptr;
    std::string last_focus_appid;
    std::string last_resume_appid;
    guint resume_timeout = 0;
    std::shared_ptr<ubuntu::app_launch::Registry> registry;
    DbusTestProcess * keeper = nullptr;
    QProcess keeper_client;
    QTemporaryDir xdg_data_home_dir;

private:
    static void focus_cb(const gchar* appid, gpointer user_data);

    static void resume_cb(const gchar* appid, gpointer user_data);

protected:
    /* Useful debugging stuff, but not on by default.  You really want to
       not get all this noise typically */
    void debugConnection();

    void startTasks();

    virtual void SetUp() override;

    virtual void TearDown() override;

    bool init_helper_registry(QString const& registry);

    bool startKeeperClient();

    int checkStorageFrameworkNbFiles();

    bool checkStorageFrameworkFiles(QStringList const & sourceDirs, bool compression=false);

    bool checkLastStorageFrameworkFile (QString const & sourceDir, bool compression=false);

    bool compareTarContent (QString const & tarPath, QString const & sourceDir, bool compression);

    bool extractTarContents(QString const & tarPath, QString const & destination, bool compression=false);

    QString getLastStorageFrameworkFile();

    bool checkStorageFrameworkContent(QString const & content);

    bool removeHelperMarkBeforeStarting();

    bool waitUntilHelperFinishes(QString const & app_id, int maxTimeout = 15000, int times = 1);

    void sendUpstartHelperTermination(QString const &app_id);

    QString getUUIDforXdgFolderPath(QString const &path, QVariantDictMap const & choices) const;

    GVariant* find_env(GVariant* env_array, const gchar* var);

    std::string get_env(GVariant* env_array, const gchar* key);

    bool have_env(GVariant* env_array, const gchar* key);

    void pause(guint time = 0);
};

#define EXPECT_ENV(expected, envvars, key) EXPECT_EQ(expected, get_env(envvars, key)) << "for key " << key
#define ASSERT_ENV(expected, envvars, key) ASSERT_EQ(expected, get_env(envvars, key)) << "for key " << key
