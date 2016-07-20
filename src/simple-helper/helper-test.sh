#!/bin/bash
#
# Copyright (C) 2016 Canonical, Ltd.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranties of
# MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authors:
#     Xavi Garcia <xavi.garcia.mena@canonical.com>
#
DIR_TO_BACKUP=$1
PATH_TO_KEEPER_TAR_CREATE=$2

find $DIR_TO_BACKUP -type f -print0 | ${PATH_TO_KEEPER_TAR_CREATE} -0 -a /com/canonical/keeper/helper
touch /tmp/simple-helper-finished
