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
 *   Xavi Garcia Mena <xavi.garcia.mena@canonical.com>
 */


#include <service/manifest.h>
#include <storage-framework/storage_framework_client.h>

#include "tests/utils/storage-framework-local.h"
#include "tests/utils/xdg-user-dirs-sandbox.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>

#include <gtest/gtest.h>
#include <glib.h>

TEST(ManifestClass, AddEntries)
{
    QString test_dir = QStringLiteral("test_dir");

    QSharedPointer<StorageFrameworkClient> sf_client(new StorageFrameworkClient);
    Manifest manifest(sf_client, test_dir);

    auto objects_to_test = 10;

    QVector<Metadata> original_metadata;
    for (auto i = 0; i < objects_to_test; ++i)
    {
        auto uuid_str = QString("%1").arg(i);
        auto display_name = QString("This is the display name for %1").arg(i);
        auto prop_1_key = QString("%1-prop1").arg(i);
        auto prop_1_val = QString("%1-prop1-value").arg(i);
        auto prop_2_key = QString("%1-prop2").arg(i);
        auto prop_2_val = QString("%1-prop2-value").arg(i);

        Metadata metadata(uuid_str, display_name);
        metadata.set_property(prop_1_key, prop_1_val);
        metadata.set_property(prop_2_key, prop_2_val);
        original_metadata.push_back(metadata);
        manifest.add_entry(metadata);
    }

    auto stored_metadata = manifest.get_entries();

    ASSERT_EQ(stored_metadata.size(), original_metadata.size());

    for (auto i = 0; i < original_metadata.size(); ++i)
    {
        EXPECT_EQ(stored_metadata[i].uuid(), original_metadata[i].uuid());
        EXPECT_EQ(stored_metadata[i].display_name(), original_metadata[i].display_name());
        EXPECT_EQ(stored_metadata[i].get_public_properties(), original_metadata[i].get_public_properties());
    }
}

TEST(ManifestClass, StoreTest)
{
    QTemporaryDir tmp_dir;
    QString test_dir = QStringLiteral("test_dir");

    g_setenv("XDG_DATA_HOME", tmp_dir.path().toLatin1().data(), true);

    QSharedPointer<StorageFrameworkClient> sf_client(new StorageFrameworkClient);
    Manifest manifest(sf_client, test_dir);

    auto objects_to_test = 10;

    QVector<Metadata> original_metadata;
    for (auto i = 0; i < objects_to_test; ++i)
    {
        auto uuid_str = QString("%1").arg(i);
        auto display_name = QString("This is the display name for %1").arg(i);
        auto prop_1_key = QString("%1-prop1").arg(i);
        auto prop_1_val = QString("%1-prop1-value").arg(i);
        auto prop_2_key = QString("%1-prop2").arg(i);
        auto prop_2_val = QString("%1-prop2-value").arg(i);

        Metadata metadata(uuid_str, display_name);
        metadata.set_property(prop_1_key, prop_1_val);
        metadata.set_property(prop_2_key, prop_2_val);
        original_metadata.push_back(metadata);
        manifest.add_entry(metadata);
    }

    QSignalSpy spy(&manifest, &Manifest::finished);

    manifest.store();

    // wait for the manifest to be stored
    spy.wait();

    ASSERT_EQ(spy.count(), 1);

    QList<QVariant> arguments = spy.takeFirst();
    EXPECT_TRUE(arguments.at(0).toBool());

    // check the file created
    auto sf_files = StorageFrameworkLocalUtils::get_storage_framework_files();
    ASSERT_EQ(sf_files.size(), 1);

    // check the file content
    EXPECT_EQ(sf_files.at(0).fileName(), "manifest.json"); // TODO define the file name in the header?
    QFile file(sf_files.at(0).absoluteFilePath());
    ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text)) << qPrintable(file.errorString());
    auto content_file = file.readAll();
    file.close();

    // Test that the stored data is OK
    // This is a basic check of the data reading the contents of the file without using the
    // read() method in Manifest.
    auto doc_read = QJsonDocument::fromJson(content_file);

    auto json_read_root = doc_read.object();
    auto items = json_read_root["entries"].toArray(); // TODO question.. should be define this key at the header?

    QVector<Metadata> read_metadata;
    for( auto iter = items.begin(); iter != items.end(); ++iter)
    {
        read_metadata.push_back(Metadata((*iter).toObject()));
    }

    ASSERT_EQ(read_metadata.size(), original_metadata.size());

    for (auto i = 0; i < read_metadata.size(); ++i)
    {
        EXPECT_EQ(read_metadata[i].uuid(), original_metadata[i].uuid());
        EXPECT_EQ(read_metadata[i].display_name(), original_metadata[i].display_name());
        EXPECT_EQ(read_metadata[i].get_public_properties(), original_metadata[i].get_public_properties());
    }

    //
    // now read the manifest with storage-framework
    //
    Manifest manifest_read(sf_client, test_dir);
    QSignalSpy spy_read(&manifest_read, &Manifest::finished);

    manifest_read.read();

    // wait for the manifest to be read
    spy_read.wait();

    ASSERT_EQ(spy_read.count(), 1);
    arguments = spy_read.takeFirst();
    EXPECT_TRUE(arguments.at(0).toBool());

    auto metadata_with_sf = manifest_read.get_entries();

    ASSERT_EQ(metadata_with_sf.size(), original_metadata.size());

    for (auto i = 0; i < metadata_with_sf.size(); ++i)
    {
        EXPECT_EQ(metadata_with_sf[i].uuid(), original_metadata[i].uuid());
        EXPECT_EQ(metadata_with_sf[i].display_name(), original_metadata[i].display_name());
        EXPECT_EQ(metadata_with_sf[i].get_public_properties(), original_metadata[i].get_public_properties());
    }

    g_unsetenv("XDG_DATA_HOME");
}
