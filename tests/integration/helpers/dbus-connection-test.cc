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
#include "tests/fakes/fake-restore-helper.h"

class TestHelpers: public TestHelpersBase
{
    using super = TestHelpersBase;

    void SetUp() override
    {
        super::SetUp();
//        init_helper_registry(HELPER_REGISTRY);
    }
};

TEST_F(TestHelpers, StartDbus)
{
    system("ls -la /usr/share/");
    system("ls -la /usr/share/libqtdbustest");
    auto session_str = QStringLiteral("DBUS_SESSION_BUS_ADDRESS=%1").arg(dbus_test_runner.sessionBus());
    qDebug() << session_str;
//    g_setenv("DBUS_SYSTEM_BUS_ADDRESS", dbus_test_runner.systemBus().toStdString().c_str(), true);
//    g_setenv("DBUS_SESSION_BUS_ADDRESS", dbus_test_runner.sessionBus().toStdString().c_str(), true);
    while(1){}
}

