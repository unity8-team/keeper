/*
 * Copyright 2016 Canonical Ltd.
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
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#include "qdbus-stubs/dbus-types.h"
#include "qdbus-stubs/keeper_interface.h"
#include "qdbus-stubs/keeper_user_interface.h"

#include <gtest/gtest.h>

#include <libqtdbusmock/DBusMock.h>
#include <libqtdbustest/DBusTestRunner.h>

#include <QDBusConnection>
#include <QDebug>
#include <QString>
#include <QThread>

#include <memory>

using namespace QtDBusMock;
using namespace QtDBusTest;

namespace
{

QVariantMap toQVariantMap(const QMap<QString,QMap<QString,QVariant>>& in)
{
    QVariantMap out;
    for (auto it=in.cbegin(), end=in.cend(); it!=end; ++it)
        out.insert(it.key(), it.value());
    return out;
}

}

class KeeperTemplateTest : public ::testing::Test
{
protected:

    virtual void SetUp() override
    {
        test_runner_.reset(new DBusTestRunner{});

        QVariant v = QVariant::fromValue(backup_choices);
        ASSERT_TRUE(v.isValid());

        // manual conversion to QVariantMap,
        //not sure why this can't happen automatically... :P
        QVariantMap helpers_qvm;
        for (auto it=helpers.cbegin(), end=helpers.cend(); it!=end; ++it)
            helpers_qvm.insert(it.key(), it.value());

        dbus_mock_.reset(new DBusMock(*test_runner_));
        dbus_mock_->registerTemplate(
            DBusTypes::KEEPER_SERVICE,
            SERVICE_TEMPLATE_FILE,
            QVariantMap {
                { "backup-choices", toQVariantMap(backup_choices) },
                { "helpers", helpers_qvm },
                { "restore-choices", toQVariantMap(restore_choices) }
            },
            QDBusConnection::SessionBus
        );

        test_runner_->startServices();
        auto conn = connection();
        qDebug() << "session bus is" << qPrintable(conn.name());

        keeper_iface_.reset(
            new DBusInterfaceKeeper(
                DBusTypes::KEEPER_SERVICE,
                DBusTypes::KEEPER_SERVICE_PATH,
                conn
            )
        );
        ASSERT_TRUE(keeper_iface_->isValid()) << qPrintable(conn.lastError().message());

        user_iface_.reset(
            new DBusInterfaceKeeperUser(
                DBusTypes::KEEPER_SERVICE,
                DBusTypes::KEEPER_USER_PATH,
                conn
            )
        );
        ASSERT_TRUE(user_iface_->isValid()) << qPrintable(conn.lastError().message());
    }

    virtual void TearDown() override
    {
        user_iface_.reset();
        keeper_iface_.reset();
        dbus_mock_.reset();
        test_runner_.reset();
    }

    const QDBusConnection& connection()
    {
        return test_runner_->sessionConnection();
    }

    std::unique_ptr<DBusTestRunner> test_runner_;
    std::unique_ptr<DBusMock> dbus_mock_;
    std::unique_ptr<DBusInterfaceKeeper> keeper_iface_;
    std::unique_ptr<DBusInterfaceKeeperUser> user_iface_;

    void EXPECT_EVENTUALLY(std::function<bool()>&& test, qint64 timeout_msec=5000)
    {
        QElapsedTimer timer;
        timer.start();
        bool passed;
        do {
            passed = test();
            if (!passed)
                QThread::usleep(200000);
        } while (!passed && !timer.hasExpired(timeout_msec));
        EXPECT_TRUE(passed);
    }

    /***
    ****
    ****  The template parameters
    ****
    ***/

    // FIXME: these types are very likely to change soon:
    // Design wants more granularity for system data, eg texts vs mail vs contacts.
    // Waiting on final design now.
    const QMap<QString,QString> helpers {
        {"application", "app-helper-stub"},
        {"folder", FOLDER_HELPER_EXEC },
        {"system-data", "system-data-helper-stub"}
    };

    const QMap<QString,QMap<QString,QVariant>> backup_choices {
        {"P9qfw0kfNyXgjAZLyViY", QVariantMap{{"type", "folder"}, {"display-name", "Documents"}}},
        {"EHdhNs26cLxllMFH6MXy", QVariantMap{{"type", "folder"}, {"display-name", "Movies"}}},
        {"z0sWRL4xWMAulDLcnPSQ", QVariantMap{{"type", "folder"}, {"display-name", "Music"}}},
        {"sRVJlx00QyalHUvsBrWo", QVariantMap{{"type", "folder"}, {"display-name", "Pictures"}}},
    };

    const QMap<QString,QMap<QString,QVariant>> restore_choices {
        {"LETjGClJ8qxoEEp2lP2l", {{"size", quint64(1000)}, {"ctime", quint64(1451606400/*2016-01-01 00:00:00*/)}, {"display-name", "Documents"}}},
        {"F1pG9zYCKCYlsXecNdJv", {{"size", quint64(1000)}, {"ctime", quint64(1451692800/*2016-01-02 00:00:00*/)}, {"display-name", "Movies"}}},
        {"8WDJNx7eUMsPfZ8kIsQr", {{"size", quint64(1000)}, {"ctime", quint64(1451779200/*2016-01-03 00:00:00*/)}, {"display-name", "Music"}}},
        {"nVQRz48gqPUnsL3inFsO", {{"size", quint64(1000)}, {"ctime", quint64(1451865600/*2016-01-04 00:00:00*/)}, {"display-name", "Pictures"}}}
    };

};

/***
****  Quick surface-level tests
***/

/* Test that the dbusmock scaffolding starts up and exits ok */
TEST_F(KeeperTemplateTest, HelloWorld)
{
}

/* Test that BackupChoices() returns the choices we created the template with */
TEST_F(KeeperTemplateTest, BackupChoices)
{
    auto pending_reply = user_iface_->GetBackupChoices();
    pending_reply.waitForFinished();
    EXPECT_TRUE(pending_reply.isValid()) << qPrintable(connection().lastError().message());
    EXPECT_EQ(backup_choices, pending_reply.value());
}

/* Test that StartBackup() fails if we pass an invalid arg */
TEST_F(KeeperTemplateTest, StartBackupWithInvalidArg)
{
    const auto invalid_uuid = QStringLiteral("yccXoXICC0ugEhMJCkQN");
    ASSERT_FALSE(backup_choices.contains(invalid_uuid));

    auto pending_reply = user_iface_->StartBackup(QStringList{invalid_uuid});
    pending_reply.waitForFinished();
    auto error = pending_reply.error();
    EXPECT_FALSE(pending_reply.isValid()) << qPrintable(error.message());
    EXPECT_TRUE(error.isValid());
    EXPECT_EQ(QDBusError::InvalidArgs, error.type());
}

/* Test that RestoreChoices() returns the choices we created the template with */
TEST_F(KeeperTemplateTest, RestoreChoices)
{
    auto pending_reply = user_iface_->GetRestoreChoices();
    pending_reply.waitForFinished();
    EXPECT_TRUE(pending_reply.isValid()) << qPrintable(connection().lastError().message());
    EXPECT_EQ(restore_choices, pending_reply.value());
}

/* Test that StartRestore() fails if we pass an invalid arg */
TEST_F(KeeperTemplateTest, StartRestoreWithInvalidArg)
{
    const auto invalid_uuid = QStringLiteral("meJjH5H2yWrmeTkjqNhw");
    ASSERT_FALSE(restore_choices.contains(invalid_uuid));

    auto pending_reply = user_iface_->StartRestore(QStringList{invalid_uuid});
    pending_reply.waitForFinished();
    auto error = pending_reply.error();
    EXPECT_FALSE(pending_reply.isValid()) << qPrintable(error.message());
    EXPECT_TRUE(error.isValid());
    EXPECT_EQ(QDBusError::InvalidArgs, error.type());
}

/* Test that Status() returns empty if we haven't done anything yet */
TEST_F(KeeperTemplateTest, TestEmptyStatus)
{
    EXPECT_TRUE(user_iface_->state().isEmpty());
}

/***
****  Okay let's try a backup
***/

// FIXME: this should go in a shared header
enum {
    ACTION_QUEUED = 0,
    ACTION_SAVING = 1,
    ACTION_RESTORING = 2,
    ACTION_IDLE = 3
};

TEST_F(KeeperTemplateTest, SingleFolderBackup)
{
    // get the pictures uuid
    const auto uuid = QStringLiteral("sRVJlx00QyalHUvsBrWo");
    ASSERT_EQ(QString::fromUtf8("Pictures"), backup_choices.value(uuid).value("display-name"));

    // start backing it up
    auto state = user_iface_->state();
    auto pending_reply = user_iface_->StartBackup(QStringList{uuid});
    pending_reply.waitForFinished();
    EXPECT_TRUE(pending_reply.isValid()) << qPrintable(pending_reply.error().message());

    // wait for the task's percent_done to reach 100%
    EXPECT_EVENTUALLY([this,uuid]{
        const auto state = user_iface_->state();
        const auto task = state.value(uuid);
        const auto percent_done = task.value("percent-done").toDouble();
        const auto action = task.value("action", -1).toInt();
        qInfo() << "action" << action << "percent_done" << percent_done;
        return int(percent_done)==1 && action==ACTION_IDLE;
    });
}
