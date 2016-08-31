/*
 * Copyright (C) 2016 Canonical, Ltd.
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
 *     Xavi Garcia Mena <xavi.garcia.mena@canonical.com>
 */
#pragma once

namespace
{
    constexpr char const UPSTART_SERVICE[] = "com.ubuntu.Upstart";
    constexpr char const UPSTART_INTERFACE[] = "com.ubuntu.Upstart0_6";
    constexpr char const UPSTART_PATH[] = "/com/ubuntu/Upstart";

    constexpr char const UPSTART_HELPER_JOB_INTERFACE[] = "com.ubuntu.Upstart0_6.Job";
    constexpr char const UPSTART_HELPER_JOB_PATH[] = "/com/test/untrusted/helper";

    constexpr char const UPSTART_HELPER_INSTANCE_PATH[] = "/com/test/untrusted/helper/instance";
    constexpr char const UPSTART_HELPER_INSTANCE_NAME[] = "untrusted-type::com.foo_bar_43.23.1";

    constexpr char const UPSTART_HELPER_MULTI_INSTANCE_PATH[] = "/com/test/untrusted/helper/multi_instance";
    constexpr char const UPSTART_HELPER_MULTI_INSTANCE_NAME[] = "backup-helper:24034582324132:com.bar_foo_8432.13.1";
}
