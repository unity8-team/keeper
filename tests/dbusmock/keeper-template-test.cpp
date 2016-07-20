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

#include "fake-backup-helper.h"

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
#include <QUuid>
#include <QVariant>

#include <memory>

#define KEY_ACTION "action"
#define KEY_CTIME "ctime"
#define KEY_BLOB "blob-data"
#define KEY_HELPER "helper-exec"
#define KEY_ICON "icon"
#define KEY_NAME "display-name"
#define KEY_PERCENT "percent-done"
#define KEY_SIZE "size"
#define KEY_SUBTYPE "subtype"
#define KEY_TYPE "type"
#define KEY_UUID "uuid"

// FIXME: this should go in a shared header
enum
{
    ACTION_QUEUED = 0,
    ACTION_SAVING = 1,
    ACTION_RESTORING = 2,
    ACTION_IDLE = 3
};

using namespace QtDBusMock;
using namespace QtDBusTest;


class KeeperTemplateTest : public ::testing::Test
{
protected:

    virtual void SetUp() override
    {
        test_runner_.reset(new DBusTestRunner{});

        dbus_mock_.reset(new DBusMock(*test_runner_));
        dbus_mock_->registerTemplate(
            DBusTypes::KEEPER_SERVICE,
            SERVICE_TEMPLATE_FILE,
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

        mock_iface_.reset(
            new QDBusInterface(
                DBusTypes::KEEPER_SERVICE,
                DBusTypes::KEEPER_SERVICE_PATH,
                QStringLiteral("com.canonical.keeper.Mock"),
                conn
            )
        );
        ASSERT_TRUE(mock_iface_->isValid()) << qPrintable(conn.lastError().message());
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
    std::unique_ptr<QDBusInterface> mock_iface_;

    void EXPECT_EVENTUALLY(std::function<bool()>&& test, qint64 timeout_msec=5000)
    {
        QElapsedTimer timer;
        timer.start();
        bool passed;
        do {
            passed = test();
            if (!passed)
                QThread::msleep(100);
        } while (!passed && !timer.hasExpired(timeout_msec));
        EXPECT_TRUE(passed);
    }

    void wait_for_backup_to_finish()
    {
        EXPECT_EVENTUALLY([this]{return !user_iface_->state().isEmpty();}); // backup running
        EXPECT_EVENTUALLY([this]{return user_iface_->state().isEmpty();}); // backup finished
    }
};


/***
****  Quick surface-level tests
***/


// test that the dbusmock scaffolding starts up and exits ok
TEST_F(KeeperTemplateTest, HelloWorld)
{
}


// test GetBackupChoices() returns what we give to AddBackupChoice()
TEST_F(KeeperTemplateTest, BackupChoices)
{
    // build a backup choice
    const auto uuid = QUuid::createUuid().toString();
    const QMap<QString,QVariant> props {
        { KEY_NAME, QStringLiteral("some-name") },
        { KEY_TYPE, QStringLiteral("some-type") },
        { KEY_SUBTYPE, QStringLiteral("some-subtype") },
        { KEY_ICON, QStringLiteral("some-icon") },
        { KEY_HELPER, QString::fromUtf8(FAKE_BACKUP_HELPER_EXEC) }
    };

    // add it
    auto msg = mock_iface_->call(QStringLiteral("AddBackupChoice"), uuid, props);
    EXPECT_NE(QDBusMessage::ErrorMessage, msg.type()) << qPrintable(msg.errorMessage());

    // ask for a list of backup choices
    QDBusReply<QVariantDictMap> choices = user_iface_->call("GetBackupChoices");
    EXPECT_TRUE(choices.isValid()) << qPrintable(choices.error().message());

    // check the results
    const auto expected_choices = QVariantDictMap{{uuid, props}};
    ASSERT_EQ(expected_choices, choices);
}


// test that StartBackup() fails if we pass an invalid arg
TEST_F(KeeperTemplateTest, StartBackupWithInvalidArg)
{
    const auto invalid_uuid = QUuid::createUuid().toString();

    QDBusReply<void> reply = user_iface_->call("StartBackup", QStringList{invalid_uuid});
    EXPECT_FALSE(reply.isValid());
    EXPECT_EQ(QDBusError::InvalidArgs, reply.error().type());
}


// test GetRestoreChoices() returns what we give to AddRestoreChoice()
TEST_F(KeeperTemplateTest, RestoreChoices)
{
    // build a restore choice
    const auto uuid = QUuid::createUuid().toString();
    const auto blob = QUuid::createUuid().toByteArray();
    const QMap<QString,QVariant> props {
        { KEY_NAME, QStringLiteral("some-name") },
        { KEY_TYPE, QStringLiteral("some-type") },
        { KEY_SUBTYPE, QStringLiteral("some-subtype") },
        { KEY_ICON, QStringLiteral("some-icon") },
        { KEY_HELPER, QString::fromUtf8("/dev/null") },
        { KEY_SIZE, quint64(blob.size()) },
        { KEY_CTIME, quint64(time(nullptr)) },
        { KEY_BLOB, blob }
    };

    // add it
    auto msg = mock_iface_->call(QStringLiteral("AddRestoreChoice"), uuid, props);
    EXPECT_NE(QDBusMessage::ErrorMessage, msg.type()) << qPrintable(msg.errorMessage());

    // ask for a list of restore choices
    QDBusReply<QVariantDictMap> choices = user_iface_->call("GetRestoreChoices");
    EXPECT_TRUE(choices.isValid()) << qPrintable(choices.error().message());

    // check the results
    const auto expected_choices = QVariantDictMap{{uuid, props}};
    ASSERT_EQ(expected_choices, choices);
}


// test that StartRestore() fails if we pass an invalid arg
TEST_F(KeeperTemplateTest, StartRestoreWithInvalidArg)
{
    const auto invalid_uuid = QUuid::createUuid().toString();

    QDBusReply<void> reply = user_iface_->call("StartRestore", QStringList{invalid_uuid});
    EXPECT_FALSE(reply.isValid());
    EXPECT_EQ(QDBusError::InvalidArgs, reply.error().type());
}


// test that Status() returns empty if we haven't done anything yet
TEST_F(KeeperTemplateTest, TestEmptyStatus)
{
    EXPECT_TRUE(user_iface_->state().isEmpty());
}


/***
****  Make a real backup
***/

TEST_F(KeeperTemplateTest, BackupRun)
{
    // build a backup choice
    const auto uuid = QUuid::createUuid().toString();
    const QMap<QString,QVariant> props {
        { KEY_NAME, QStringLiteral("Music") },
        { KEY_TYPE, QStringLiteral("folder") },
        { KEY_SUBTYPE, QStringLiteral("/home/charles/Music") },
        { KEY_ICON, QStringLiteral("music-icon") },
        { KEY_HELPER, QString::fromUtf8(FAKE_BACKUP_HELPER_EXEC) }
    };

    // add it
    auto msg = mock_iface_->call(QStringLiteral("AddBackupChoice"), uuid, props);
    EXPECT_NE(QDBusMessage::ErrorMessage, msg.type()) << qPrintable(msg.errorMessage());

    // start the backup
    QDBusReply<void> reply = user_iface_->call("StartBackup", QStringList{uuid});
    EXPECT_TRUE(reply.isValid()) << qPrintable(reply.error().message());
    wait_for_backup_to_finish();

    // ask keeper for the blob
    QDBusReply<QByteArray> blob = mock_iface_->call(QStringLiteral("GetBackupData"), uuid);
    EXPECT_TRUE(reply.isValid()) << qPrintable(reply.error().message());

    // check the results
    const auto expected_blob = QByteArray(FAKE_BACKUP_HELPER_PAYLOAD);
    ASSERT_EQ(expected_blob, blob);
}
