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

#include "src/qdbus-stubs/dbus-types.h"
#include "src/qdbus-stubs/keeper_user_interface.h"

#include <gtest/gtest.h>

#include <libqtdbusmock/DBusMock.h>
#include <libqtdbustest/DBusTestRunner.h>

#include <QDBusConnection>
#include <QDebug>
#include <QString>
#include <QThread>
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

// FIXME: this should go in a shared header in include/
enum
{
    ACTION_QUEUED = 0,
    ACTION_SAVING = 1,
    ACTION_RESTORING = 2,
    ACTION_IDLE = 3
};

using namespace QtDBusMock;
using namespace QtDBusTest;


class KeeperDBusMockFixture : public ::testing::Test
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
        dbus_mock_.reset();
        test_runner_.reset();
    }

    const QDBusConnection& connection()
    {
        return test_runner_->sessionConnection();
    }

    std::unique_ptr<DBusTestRunner> test_runner_;
    std::unique_ptr<DBusMock> dbus_mock_;
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

    QString add_backup_choice(const QMap<QString,QVariant>& properties)
    {
        const auto uuid = QUuid::createUuid().toString();
        auto msg = mock_iface_->call(QStringLiteral("AddBackupChoice"), uuid, properties);
        EXPECT_NE(QDBusMessage::ErrorMessage, msg.type()) << qPrintable(msg.errorMessage());
        return uuid;
    }
};
