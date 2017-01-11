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
 *   Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#include <storage-framework/storage_framework_client.h>

#include "tests/utils/storage-framework-local.h"
#include "tests/utils/xdg-user-dirs-sandbox.h"

#include <QSignalSpy>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include <cstdlib> // setenv(), unsetenv()

namespace sf = unity::storage::qt::client;

TEST(SF, FoldersTest)
{
    QTemporaryDir tmp_dir;
    QString test_dir = QStringLiteral("test_dir");
    QVector<QString> test_dirs;

    auto nb_tests = 10;
    for (auto i = 0; i < nb_tests; ++i)
    {
        test_dirs.push_back(QString("test_dir_%1").arg(i+1));
    }

    setenv("XDG_DATA_HOME", tmp_dir.path().toLatin1().data(), true);
    StorageFrameworkClient sf_client;

    for (auto i = 0; i < nb_tests; ++i)
    {
        auto uploader_fut = sf_client.get_new_uploader(0, test_dirs.at(i), "just_to_create_keeper_folder");
        {
            QFutureWatcher<std::shared_ptr<Uploader>> w;
            QSignalSpy spy(&w, &decltype(w)::finished);
            w.setFuture(uploader_fut);
            assert(spy.wait());
            ASSERT_EQ(spy.count(), 1);
        }
        auto uploader = uploader_fut.result();
        ASSERT_NE(uploader, nullptr);
        // dispose the uploader, we just wanted it to create the directory.
        uploader.reset();
    }

    QDir sf_dir;
    ASSERT_TRUE(StorageFrameworkLocalUtils::find_storage_framework_root_dir(sf_dir));
    qDebug() << "SF PATH: " << sf_dir.absolutePath();
    ASSERT_TRUE(sf_dir.exists());

    auto entry_list = sf_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    EXPECT_EQ(entry_list.size(), nb_tests);
    for (auto i = 0; i < entry_list.size(); ++i)
    {
        bool found = false;
        for (auto j = 0; j < test_dirs.size() && !found; ++j)
        {
            if (entry_list.at(i).baseName() == test_dirs.at(j))
            {
                found = true;
            }
        }
        EXPECT_TRUE(found);
    }

    auto folders_fut = sf_client.get_keeper_dirs();
    {
        QFutureWatcher<QVector<QString>> w;
        QSignalSpy spy(&w, &decltype(w)::finished);
        w.setFuture(folders_fut);
        assert(spy.wait());
        ASSERT_EQ(spy.count(), 1);
    }

    auto sf_dirs = folders_fut.result();
    EXPECT_EQ(sf_dirs.size(), nb_tests);
    for (auto i = 0; i < sf_dirs.size(); ++i)
    {
        bool found = false;
        for (auto j = 0; j < test_dirs.size() && !found; ++j)
        {
            if (sf_dirs.at(i) == test_dirs.at(j))
            {
                found = true;
            }
        }
        EXPECT_TRUE(found);
    }

    unsetenv("XDG_DATA_HOME");
}
