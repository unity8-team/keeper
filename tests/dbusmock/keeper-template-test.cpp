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

#include "tests/fakes/fake-backup-helper.h"

#include "tests/utils/keeper-dbusmock-fixture.h"

#include <QDebug>
#include <QString>
#include <QThread>
#include <QUuid>
#include <QVariant>

#include <memory>

using KeeperTemplateTest = KeeperDBusMockFixture;

/***
****  Quick surface-level tests
***/


// test that the dbusmock scaffolding starts up and exits ok
TEST_F(KeeperTemplateTest, HelloWorld)
{
    // avoid FAKE_BACKUP_HELPER_PAYLOAD from fake-backup-helper.h
    (void)FAKE_BACKUP_HELPER_PAYLOAD;
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
        { KEY_HELPER, QString::fromUtf8(FAKE_BACKUP_HELPER_EXEC) }
    };

    // add it
    auto msg = mock_iface_->call(QStringLiteral("AddBackupChoice"), uuid, props);
    EXPECT_NE(QDBusMessage::ErrorMessage, msg.type()) << qPrintable(msg.errorMessage());

    // ask for a list of backup choices
    QDBusPendingReply<keeper::Items> choices_reply = user_iface_->call("GetBackupChoices");
    choices_reply.waitForFinished();
    if (!choices_reply.isValid())
    {
        qDebug() << "-------------------" << choices_reply.error().message();
    }
    EXPECT_TRUE(choices_reply.isValid()) << qPrintable(choices_reply.error().message());

    auto choices = choices_reply.value();
    // check the results
    const auto expected_choices = QVariantDictMap{{uuid, props}};
    auto iter = choices.find(uuid);
    ASSERT_NE(iter, choices.end());

    ASSERT_EQ((*iter), props);
}


//// test that StartBackup() fails if we pass an invalid arg
//TEST_F(KeeperTemplateTest, StartBackupWithInvalidArg)
//{
//    const auto invalid_uuid = QUuid::createUuid().toString();
//
//    QDBusReply<void> reply = user_iface_->call("StartBackup", QStringList{invalid_uuid});
//    EXPECT_FALSE(reply.isValid());
//    EXPECT_EQ(QDBusError::InvalidArgs, reply.error().type());
//}
//
//
//// test GetRestoreChoices() returns what we give to AddRestoreChoice()
//TEST_F(KeeperTemplateTest, RestoreChoices)
//{
//    // build a restore choice
//    const auto uuid = QUuid::createUuid().toString();
//    const auto blob = QUuid::createUuid().toByteArray();
//    const QMap<QString,QVariant> props {
//        { KEY_NAME, QStringLiteral("some-name") },
//        { KEY_TYPE, QStringLiteral("some-type") },
//        { KEY_SUBTYPE, QStringLiteral("some-subtype") },
//        { KEY_HELPER, QString::fromUtf8("/dev/null") },
//        { KEY_SIZE, quint64(blob.size()) },
//        { KEY_CTIME, quint64(time(nullptr)) },
//        { KEY_BLOB, blob }
//    };
//
//    // add it
//    auto msg = mock_iface_->call(QStringLiteral("AddRestoreChoice"), uuid, props);
//    EXPECT_NE(QDBusMessage::ErrorMessage, msg.type()) << qPrintable(msg.errorMessage());
//
//    // ask for a list of restore choices
//    QDBusReply<QVariantDictMap> choices = user_iface_->call("GetRestoreChoices");
//    EXPECT_TRUE(choices.isValid()) << qPrintable(choices.error().message());
//
//    // check the results
//    const auto expected_choices = QVariantDictMap{{uuid, props}};
//    ASSERT_EQ(expected_choices, choices);
//}
//
//
//// test that StartRestore() fails if we pass an invalid arg
//TEST_F(KeeperTemplateTest, StartRestoreWithInvalidArg)
//{
//    const auto invalid_uuid = QUuid::createUuid().toString();
//
//    QDBusReply<void> reply = user_iface_->call("StartRestore", QStringList{invalid_uuid});
//    EXPECT_FALSE(reply.isValid());
//    EXPECT_EQ(QDBusError::InvalidArgs, reply.error().type());
//}
//
//
//// test that Status() returns empty if we haven't done anything yet
//TEST_F(KeeperTemplateTest, TestEmptyStatus)
//{
//    EXPECT_TRUE(user_iface_->state().isEmpty());
//}
//
//
///***
//****  Make a real backup
//***/
//
//TEST_F(KeeperTemplateTest, BackupRun)
//{
//    QTemporaryDir sandbox;
//
//    // build a backup choice
//    const auto uuid = QUuid::createUuid().toString();
//    const QMap<QString,QVariant> props {
//        { KEY_NAME, QStringLiteral("Music") },
//        { KEY_TYPE, QStringLiteral("folder") },
//        { KEY_SUBTYPE, sandbox.path() },
//        { KEY_HELPER, QString::fromUtf8(FAKE_BACKUP_HELPER_EXEC) }
//    };
//
//    // add it
//    auto msg = mock_iface_->call(QStringLiteral("AddBackupChoice"), uuid, props);
//    EXPECT_NE(QDBusMessage::ErrorMessage, msg.type()) << qPrintable(msg.errorMessage());
//
//    // start the backup
//    QDBusReply<void> reply = user_iface_->call("StartBackup", QStringList{uuid});
//    EXPECT_TRUE(reply.isValid()) << qPrintable(reply.error().message());
//    ASSERT_TRUE(wait_for_tasks_to_finish());
//
//    // ask keeper for the blob
//    QDBusReply<QByteArray> blob = mock_iface_->call(QStringLiteral("GetBackupData"), uuid);
//    EXPECT_TRUE(reply.isValid()) << qPrintable(reply.error().message());
//
//    // check the results
//    const auto expected_blob = QByteArray(FAKE_BACKUP_HELPER_PAYLOAD);
//    ASSERT_EQ(expected_blob, blob);
//}
