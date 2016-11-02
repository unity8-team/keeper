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


#include <helper/metadata.h>

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>

#include <gtest/gtest.h>


TEST(MetadataClass, TestJsonSingleObject)
{
    Metadata metadata("1234", "this is the display name");
    metadata.set_property("prop1", "prop1-value");
    metadata.set_property("prop2", "prop2s-value");

    auto metadata_json = metadata.json();

    QJsonDocument doc(metadata_json);
    auto str_json(doc.toJson(QJsonDocument::Compact));

    qDebug() << "JSON: " << str_json;

    auto doc_read = QJsonDocument::fromJson(str_json);

    Metadata metadata_read(doc_read.object());

    EXPECT_EQ(metadata.uuid(), metadata_read.uuid());
    EXPECT_EQ(metadata.display_name(), metadata_read.display_name());
    EXPECT_EQ(metadata.get_public_properties(), metadata_read.get_public_properties());
}

TEST(MetadataClass, TestJsonMultipleObjects)
{
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
    }

    QJsonArray json_array;
    for (auto metadata : original_metadata)
    {
        json_array.append(metadata.json());
    }
    QJsonObject json_root;
    json_root["items"] = json_array;

    QJsonDocument doc(json_root);
    auto str_json(doc.toJson(QJsonDocument::Compact));

    qDebug() << "JSON: " << str_json;

    auto doc_read = QJsonDocument::fromJson(str_json);

    auto json_read_root = doc_read.object();
    auto items = json_read_root["items"].toArray();

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
}
