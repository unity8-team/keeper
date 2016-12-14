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

#include "tar/tar-creator.h"

#include <gtest/gtest.h>

#include <QString>
#include <QTemporaryDir>


/***
****
***/

class KeeperUntarFixture: public KeeperDBusMockFixture
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

protected:

    std::vector<char> tar_directory_into_memory(QString const & in_path)
    {
        std::vector<char> blob;

        auto const currentPath = QDir::currentPath();
        EXPECT_TRUE(QDir::setCurrent(in_path));

        QDir const indir(in_path);
        QStringList files;
        for (auto file : FileUtils::getFilesRecursively(in_path))
            files += indir.relativeFilePath(file);

        TarCreator tar_creator(files, false);
        std::vector<char> step;
        while (tar_creator.step(step))
            blob.insert(blob.end(), step.begin(), step.end());

        EXPECT_TRUE(QDir::setCurrent(currentPath));

        return blob;
    }

    QMap<QString,QVariant> build_folder_restore_choice(
        QTemporaryDir& source,
        QTemporaryDir& target,
        char const* helper_exec,
        std::vector<char> const& blob)
    {

        return QMap<QString,QVariant>{
            { KEY_NAME, QDir(source.path()).dirName() },
            { KEY_TYPE, QStringLiteral("folder") },
            { KEY_SUBTYPE, target.path() },
            { KEY_HELPER, QString::fromUtf8(helper_exec) },
            { KEY_SIZE, quint64(blob.size()) },
            { KEY_CTIME, quint64(time(nullptr)) },
            { KEY_BLOB, QByteArray{&blob.front(), blob.size()} }
        };
    }

    void restore(QString const& uuid)
    {
        QDBusReply<void> reply = user_iface_->call("StartRestore", QStringList{uuid});
        ASSERT_TRUE(reply.isValid()) << qPrintable(reply.error().message());
        ASSERT_TRUE(wait_for_tasks_to_finish());
    }
};

/***
****
***/

TEST_F(KeeperUntarFixture, RestoreRun)
{
    static constexpr int n_runs {2};

    for (int i=0; i<n_runs; ++i)
    {
        // build a directory full of random files
        QTemporaryDir in;
        FileUtils::fillTemporaryDirectory(in.path());

        // tar it up
        auto const blob = tar_directory_into_memory(in.path());

        // tell keeper that's a restore choice
        QTemporaryDir out;
        const auto uuid = add_restore_choice(build_folder_restore_choice(in, out, KU_INVOKE, blob));

        // now run the restore
        restore(uuid);

        // after restore, the source and restore dirs should match
        EXPECT_TRUE(FileUtils::compareDirectories(in.path(), out.path()));

        // if the test failed, keep the artifacts for a human to look at
        auto const passed = ::testing::UnitTest::GetInstance()->current_test_info()->result()->Passed();
        in.setAutoRemove(passed);
        out.setAutoRemove(passed);
    }
}

/***
****
***/

TEST_F(KeeperUntarFixture, BadArgNoBus)
{
    static constexpr int n_runs {1};

    for (int i=0; i<n_runs; ++i)
    {
        // build a directory full of random files
        QTemporaryDir in;
        FileUtils::fillTemporaryDirectory(in.path());

        // tar it up
        auto const blob = tar_directory_into_memory(in.path());

        // tell keeper that's a restore choice
        QTemporaryDir out;
        const auto uuid = add_restore_choice(build_folder_restore_choice(in, out, KU_INVOKE_NOBUS, blob));

        // now run the restore
        restore(uuid);

        // confirm that the backup ended in error
        const auto state = user_iface_->state();
        const auto& properties = state[uuid];
        EXPECT_EQ(QString::fromUtf8("failed"), properties.value(KEY_ACTION))
            << qPrintable(properties.value(KEY_ACTION).toString());
        EXPECT_FALSE(properties.value(KEY_ERROR).toString().isEmpty());
    }
}
