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
#include "qdbus-stubs/keeper_user_interface.h"

#include <gtest/gtest.h>

#include <libqtdbusmock/DBusMock.h>
#include <libqtdbustest/DBusTestRunner.h>

#include <QDBusConnection>
#include <QDebug>
#include <QString>
#include <QThread>
#include <QVariant>

#include <memory>

// FIXME: these should go in a shared header in include/

static constexpr char const * KEY_ACTION = {"action"};
static constexpr char const * KEY_CTIME = {"ctime"};
static constexpr char const * KEY_BLOB = {"blob-data"};
static constexpr char const * KEY_ERROR = {"error"};
static constexpr char const * KEY_HELPER = {"helper-exec"};
static constexpr char const * KEY_NAME = {"display-name"};
static constexpr char const * KEY_PERCENT = {"percent-done"};
static constexpr char const * KEY_SIZE = {"size"};
static constexpr char const * KEY_SPEED = {"speed"};
static constexpr char const * KEY_SUBTYPE = {"subtype"};
static constexpr char const * KEY_TYPE = {"type"};
static constexpr char const * KEY_UUID = {"uuid"};

static constexpr char const * ACTION_QUEUED = {"queued"};
static constexpr char const * ACTION_SAVING = {"saving"};
static constexpr char const * ACTION_RESTORING = {"restoring"};
static constexpr char const * ACTION_CANCELLED = {"cancelled"};
static constexpr char const * ACTION_FAILED = {"failed"};
static constexpr char const * ACTION_COMPLETE = {"complete"};

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

    bool wait_for(
        std::function<bool()>&& test_function,
        qint64 timeout_msec=1000,
        qint64 test_interval=100)
    {
        QElapsedTimer timer;
        timer.start();
        for(;;) {
            if (test_function())
                return true;
            if (timer.hasExpired(timeout_msec))
                return false;
            QThread::msleep(test_interval);
        }
    }

    bool wait_for_tasks_to_finish()
    {
        auto tasks_exist = [this]{
            return !user_iface_->state().isEmpty();
        };

        auto all_tasks_finished = [this]{
            const auto state = user_iface_->state();
            bool all_done = true;
            for(const auto& properties : state) {
                const auto action = properties.value(KEY_ACTION);
                bool task_done = (action == ACTION_CANCELLED) || (action == ACTION_FAILED) || (action == ACTION_COMPLETE);
                if (!task_done)
                    all_done = false;
            }
            return all_done;
        };

        return wait_for(tasks_exist) && wait_for(all_tasks_finished,5000);
    }

    QString add_backup_choice(const QMap<QString,QVariant>& properties)
    {
        const auto uuid = QUuid::createUuid().toString();
        auto msg = mock_iface_->call(QStringLiteral("AddBackupChoice"), uuid, properties);
        EXPECT_NE(QDBusMessage::ErrorMessage, msg.type()) << qPrintable(msg.errorMessage());
        return uuid;
    }
};
