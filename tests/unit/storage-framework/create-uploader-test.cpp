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
#include <glib.h>

namespace sf = unity::storage::qt::client;

TEST(SF, CreateUploaderWithCommitAndDispose)
{
    QByteArray test_content = R"(
        Dorothy was a waitress on the promenade
        She worked the night shift
        Dishwater blonde, tall and fine
        She got a lot of tips
    )";

    QTemporaryDir tmp_dir;
    QString test_dir = QStringLiteral("test_dir");
    QString test_file_name = QStringLiteral("test_file");

    g_setenv("XDG_DATA_HOME", tmp_dir.path().toLatin1().data(), true);
    qDebug() << "XDG_DATA_HOME is:" << qPrintable(tmp_dir.path());

    StorageFrameworkClient sf_client;
    auto uploader_fut = sf_client.get_new_uploader(test_content.size(), test_dir, test_file_name);
    {
        QFutureWatcher<std::shared_ptr<Uploader>> w;
        QSignalSpy spy(&w, &decltype(w)::finished);
        w.setFuture(uploader_fut);
        assert(spy.wait());
        ASSERT_EQ(spy.count(), 1);
    }
    auto uploader = uploader_fut.result();
    ASSERT_NE(uploader, nullptr);

    auto socket = uploader->socket();
    ASSERT_NE(socket, nullptr);

    socket->write(test_content);

    QSignalSpy spy_commit(uploader.get(), &Uploader::commit_finished);
    uploader->commit();

    spy_commit.wait();
    EXPECT_EQ(1, spy_commit.count());

    const auto sf_files = StorageFrameworkLocalUtils::get_storage_framework_files();
    ASSERT_EQ(sf_files.size(), 1);

    // check that the file path ends up with the keeper folder
    EXPECT_TRUE(sf_files.at(0).path().endsWith(QStringLiteral("%1%2%3").arg(StorageFrameworkClient::KEEPER_FOLDER).arg(QDir::separator()).arg(test_dir)));

    // check the file content
    EXPECT_EQ(sf_files.at(0).fileName(), test_file_name);
    QFile file(sf_files.at(0).absoluteFilePath());
    ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text)) << qPrintable(file.errorString());
    const auto content_file = file.readAll();
    EXPECT_EQ(content_file, test_content);
    file.close();

    // create a downloader
    auto downloader_fut = sf_client.get_new_downloader(test_dir, test_file_name);
    {
        QFutureWatcher<std::shared_ptr<Downloader>> w;
        QSignalSpy spy(&w, &decltype(w)::finished);
        w.setFuture(downloader_fut);
        assert(spy.wait());
        ASSERT_EQ(spy.count(), 1);
    }

    auto downloader = downloader_fut.result();
    ASSERT_NE(downloader, nullptr);

    auto socket_downloader = downloader->socket();
    ASSERT_NE(socket_downloader, nullptr);

    if (socket_downloader->atEnd())
    {
        EXPECT_TRUE(socket_downloader->waitForReadyRead(5000));
    }
    auto downloader_content = socket_downloader->readAll();

    EXPECT_EQ(downloader_content, test_content);

    QSignalSpy spy_downloader(downloader.get(), &Downloader::download_finished);
    downloader->finish();
    if (!spy_downloader.count())
    {
        spy_downloader.wait();
    }
    EXPECT_EQ(1, spy_downloader.count());

    // get another uploader
    QString test_file_name_2 = QStringLiteral("test_file2");
    uploader_fut = sf_client.get_new_uploader(test_content.size(), test_dir, test_file_name_2);
    {
        QFutureWatcher<std::shared_ptr<Uploader>> w;
        QSignalSpy spy(&w, &decltype(w)::finished);
        w.setFuture(uploader_fut);
        assert(spy.wait());
        ASSERT_EQ(spy.count(), 1);
    }
    uploader = uploader_fut.result();
    ASSERT_NE(uploader, nullptr);

    // this time we write something and then dispose the uploader
    socket = uploader->socket();
    ASSERT_NE(socket, nullptr);

    socket->write(test_content);

    uploader.reset();

    // and the number of files must be 1 as the second one was not committed
    EXPECT_EQ(StorageFrameworkLocalUtils::check_storage_framework_nb_files(), 1);
    const auto sf_files_after_dispose = StorageFrameworkLocalUtils::get_storage_framework_files();

    // check that files are exactly the same than before the second uploader
    EXPECT_EQ(sf_files, sf_files_after_dispose);

    g_unsetenv("XDG_DATA_HOME");
}
