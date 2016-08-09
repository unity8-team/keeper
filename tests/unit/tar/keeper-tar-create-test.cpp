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

#include "tests/utils/file-utils.h"
#include "tests/utils/keeper-dbusmock-fixture.h"

#include <gtest/gtest.h>

#include <QString>
#include <QTemporaryDir>


/***
****
***/

class KeeperTarCreateFixture: public KeeperDBusMockFixture
{
    using parent = KeeperDBusMockFixture;

    void SetUp() override
    {
        parent::SetUp();

        qsrand(time(nullptr));
    }

    void TearDown() override
    {
    }
};

/***
****
***/

TEST_F(KeeperTarCreateFixture, BackupRun)
{
    static constexpr int n_runs {10};

    for (int i=0; i<n_runs; ++i)
    {
        // build a directory full of random files
        QTemporaryDir in;
        FileUtils::fillTemporaryDirectory(in.path());

        // tell keeper that's a backup choice
        const auto uuid = add_backup_choice(QMap<QString,QVariant>{
            { KEY_NAME, QDir(in.path()).dirName() },
            { KEY_TYPE, QStringLiteral("folder") },
            { KEY_SUBTYPE, in.path() },
            { KEY_HELPER, QString::fromUtf8(KTC_INVOKE) }
        });

        // start the backup
        QDBusReply<void> reply = user_iface_->call("StartBackup", QStringList{uuid});
        ASSERT_TRUE(reply.isValid()) << qPrintable(reply.error().message());
        ASSERT_TRUE(wait_for_tasks_to_finish());

        // ask keeper for the blob
        QDBusReply<QByteArray> blob = mock_iface_->call(QStringLiteral("GetBackupData"), uuid);
        ASSERT_TRUE(reply.isValid()) << qPrintable(reply.error().message());

        // untar it
        QTemporaryDir out;
        QDir outdir(out.path());
        QFile tarfile(outdir.filePath("tmp.tar"));
        tarfile.open(QIODevice::WriteOnly);
        tarfile.write(blob.value());
        tarfile.close();
        QProcess untar;
        untar.setWorkingDirectory(outdir.path());
        untar.start("tar", QStringList() << "xvf" << tarfile.fileName());
        EXPECT_TRUE(untar.waitForFinished()) << qPrintable(untar.errorString());

        // after we remove the temporary tarfile, the original and copy dirs should match
        EXPECT_FALSE(FileUtils::compareDirectories(in.path(), out.path()));
        EXPECT_TRUE(tarfile.remove());
        EXPECT_TRUE(FileUtils::compareDirectories(in.path(), out.path()));
    }
}

/***
****
***/

TEST_F(KeeperTarCreateFixture, BadArgNoBus)
{
    // build a directory full of random files
    QTemporaryDir in;
    FileUtils::fillTemporaryDirectory(in.path());

    // tell keeper that's a backup choice
    const auto uuid = add_backup_choice(QMap<QString,QVariant>{
        { KEY_NAME, QDir(in.path()).dirName() },
        { KEY_TYPE, QStringLiteral("folder") },
        { KEY_SUBTYPE, in.path() },
        { KEY_HELPER, QString::fromUtf8(KTC_INVOKE_NOBUS) }
    });

    // start the backup
    QDBusReply<void> reply = user_iface_->call("StartBackup", QStringList{uuid});
    ASSERT_TRUE(reply.isValid()) << qPrintable(reply.error().message());
    EXPECT_TRUE(wait_for_tasks_to_finish());

    // confirm that the backup ended in error
    const auto state = user_iface_->state();
    const auto& properties = state[uuid];
    EXPECT_EQ(QString::fromUtf8("failed"), properties.value(KEY_ACTION))
        << qPrintable(properties.value(KEY_ACTION).toString());
    EXPECT_FALSE(properties.value(KEY_ERROR).toString().isEmpty());
}

/***
****
***/

TEST_F(KeeperTarCreateFixture, BadArgNoFiles)
{
    // build a directory full of random files
    QTemporaryDir in;
    FileUtils::fillTemporaryDirectory(in.path());

    // tell keeper that's a backup choice
    const auto uuid = add_backup_choice(QMap<QString,QVariant>{
        { KEY_NAME, QDir(in.path()).dirName() },
        { KEY_TYPE, QStringLiteral("folder") },
        { KEY_SUBTYPE, in.path() },
        { KEY_HELPER, QString::fromUtf8(KTC_INVOKE_NOFILES) }
    });

    // start the backup
    QDBusReply<void> reply = user_iface_->call("StartBackup", QStringList{uuid});
    ASSERT_TRUE(reply.isValid()) << qPrintable(reply.error().message());
    EXPECT_TRUE(wait_for_tasks_to_finish());

    // confirm that the backup ended in error
    const auto state = user_iface_->state();
    const auto& properties = state[uuid];
    EXPECT_EQ(QString::fromUtf8("failed"), properties.value(KEY_ACTION))
        << qPrintable(properties.value(KEY_ACTION).toString());
    EXPECT_FALSE(properties.value(KEY_ERROR).toString().isEmpty());
}

/***
****
***/

TEST_F(KeeperTarCreateFixture, KeeperHelperStartBackupFailure)
{
    // build a directory full of random files
    QTemporaryDir in;
    FileUtils::fillTemporaryDirectory(in.path());

    // tell keeper that's a backup choice
    const auto uuid = add_backup_choice(QMap<QString,QVariant>{
        { KEY_NAME, QDir(in.path()).dirName() },
        { KEY_TYPE, QStringLiteral("folder") },
        { KEY_SUBTYPE, in.path() },
        { KEY_HELPER, QString::fromUtf8(KTC_INVOKE) }
    });

    // start the backup
    fail_next_helper_start();
    QDBusReply<void> reply = user_iface_->call("StartBackup", QStringList{uuid});
    ASSERT_TRUE(reply.isValid()) << qPrintable(reply.error().message());
    EXPECT_TRUE(wait_for_tasks_to_finish());

    // confirm that the backup ended in error
    const auto state = user_iface_->state();
    const auto& properties = state[uuid];
    EXPECT_EQ(QString::fromUtf8("failed"), properties.value(KEY_ACTION))
        << qPrintable(properties.value(KEY_ACTION).toString());
    EXPECT_FALSE(properties.value(KEY_ERROR).toString().isEmpty());
}
