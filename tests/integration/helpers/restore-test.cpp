/*
 * Copyright 2013-2016 Canonical Ltd.
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
 *     Ted Gould <ted.gould@canonical.com>
 *     Xavi Garcia <xavi.garcia.mena@canonical.com>
 *     Charles Kerr <charles.kerr@canonical.com>
 */

#include "test-helpers-base.h"
#include <service/manifest.h>
#include <storage-framework/storage_framework_client.h>

class TestHelpers: public TestHelpersBase
{
    using super = TestHelpersBase;

    void SetUp() override
    {
        super::SetUp();
        init_helper_registry(HELPER_REGISTRY);
    }
};

TEST_F(TestHelpers, GetRestoreChoices)
{
    QString PROP_1_KEY = QStringLiteral("prop1");
    QString PROP_2_KEY = QStringLiteral("prop2");

    XdgUserDirsSandbox tmp_dir;

    // starts the services, including keeper-service
    start_tasks();

    auto nb_dirs = 3;
    QVector<Metadata> all_choices;
    for (auto idir = 0; idir < nb_dirs; ++idir)
    {
        QString test_dir = QStringLiteral("test_dir-%1").arg(idir);
        QSharedPointer<StorageFrameworkClient> sf_client(new StorageFrameworkClient);
        Manifest manifest(sf_client, test_dir);

        auto objects_to_test = 10;

        QVector<Metadata> original_metadata;
        for (auto i = 0; i < objects_to_test; ++i)
        {
            auto uuid_str = QString("%1-%2").arg(idir).arg(i);
            auto display_name = QString("This is the display name for %1-%2").arg(idir).arg(i);
            auto prop_1_val = QString("%1-%2-prop1-value").arg(idir).arg(i);
            auto prop_2_val = QString("%1-%2-prop2-value").arg(idir).arg(i);

            Metadata metadata(uuid_str, display_name);
            metadata.set_property_value(PROP_1_KEY, prop_1_val);
            metadata.set_property_value(PROP_2_KEY, prop_2_val);
            original_metadata.push_back(metadata);
            manifest.add_entry(metadata);
        }
        all_choices += original_metadata;

        QSignalSpy spy(&manifest, &Manifest::finished);

        manifest.store();

        // wait for the manifest to be stored
        spy.wait();

        ASSERT_EQ(spy.count(), 1);

        QList<QVariant> arguments = spy.takeFirst();
        EXPECT_TRUE(arguments.at(0).toBool());
    }

    QSharedPointer<DBusInterfaceKeeperUser> user_iface(new DBusInterfaceKeeperUser(
                                                            DBusTypes::KEEPER_SERVICE,
                                                            DBusTypes::KEEPER_USER_PATH,
                                                            dbus_test_runner.sessionConnection()
                                                        ) );

    ASSERT_TRUE(user_iface->isValid()) << qPrintable(dbus_test_runner.sessionConnection().lastError().message());

    // ask for a list of backup choices
    QDBusPendingReply<keeper::Items> choices_reply = user_iface->call("GetRestoreChoices");
    choices_reply.waitForFinished();
    EXPECT_TRUE(choices_reply.isFinished());
    EXPECT_TRUE(choices_reply.isValid()) << qPrintable(choices_reply.error().message());

    auto choices = choices_reply.value();
    ASSERT_EQ(all_choices.size(), choices.size());

    for (auto i = 0; i < all_choices.size(); ++i)
    {
        auto iter = choices.find(all_choices[i].get_uuid());
        ASSERT_TRUE(iter != choices.end());

        auto iter_name = (*iter).find(keeper::Item::DISPLAY_NAME_KEY);
        ASSERT_TRUE(iter_name != (*iter).end());
        EXPECT_EQ(all_choices[i].get_display_name(), (*iter_name));

        auto iter_uuid = (*iter).find(keeper::Item::UUID_KEY);
        ASSERT_TRUE(iter_uuid != (*iter).end());
        EXPECT_EQ(all_choices[i].get_uuid(), (*iter_uuid));

        auto iter_prop1 = (*iter).find(PROP_1_KEY);
        ASSERT_TRUE(iter_prop1 != (*iter).end());
        QString prop_value = all_choices[i].get_property_value(PROP_1_KEY).toString();
        EXPECT_EQ(prop_value, (*iter_prop1));

        auto iter_prop2 = (*iter).find(PROP_2_KEY);
        ASSERT_TRUE(iter_prop2 != (*iter).end());
        auto prop_value_2 = all_choices[i].get_property_value(PROP_2_KEY).toString();
        EXPECT_EQ(prop_value_2, (*iter_prop2));
    }
}
