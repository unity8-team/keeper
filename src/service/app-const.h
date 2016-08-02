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
 * Author: Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#pragma once

namespace
{
    constexpr char const DEKKO_APP_ID[] = "dekko.dekkoproject_dekko_0.6.20";
    constexpr char const HELPER_TYPE[] = "backup-helper";
    constexpr char const DEKKO_HELPER_BIN[] = "/custom/click/dekko.dekkoproject/0.6.20/backup-helper";
    constexpr char const DEKKO_HELPER_DIR[] = "/custom/click/dekko.dekkoproject/0.6.20";

    // TODO set a valid bin path (depending on the installation path)
    constexpr char const FOLDER_HELPER_BIN_PATH[] = "/tmp/folder-backup-helper.sh";
}
