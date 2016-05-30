#!/bin/sh

#
# Copyright (C) 2013 Canonical Ltd
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Michi Henning <michi.henning@canonical.com>
#

#
# Check that we have acceptable license information in our source files.
#

usage()
{
    echo "usage: check_copyright dir [ignore_dir]" >&2
    exit 2
}

[ $# -lt 1 ] && usage
[ $# -gt 2 ] && usage

ignore_pat="\\.sci$|\\.swp$|\\.bzr|debian|qmldir|HACKING|tsan-suppress|valgrind-suppress|testsong_ogg|\\.txt$|\\.xml$|\\.in$|\\.dox$|\\.html$|\\.map$|\\.aiff$|\\.gif$|\\.jpg$|\\.JPG$|\\.mp2$|\\.mp3$|\\.m4a$|\\.m4v$|\\.flac$|\\.oga$|\\.ogg$|\\.opus$|\\.png$|\\.svg$|\\.wav$|UseGSettings.cmake$"

#
# We don't use the -i option of licensecheck to add ignore_dir to the pattern because Jenkins creates directories
# with names that contain regex meta-characters, such as "." and "+". Instead, if ignore_dir is set, we post-filter
# the output with grep -F, so we don't get false positives from licensecheck.
#

[ $# -eq 2 ] && ignore_dir="$2"

if [ -n "$ignore_dir" ]
then
    licensecheck -i "$ignore_pat" -r "$1" | grep -F "$ignore_dir" -v | grep 'No copyright'
else
    licensecheck -i "$ignore_pat" -r "$1" | grep 'No copyright'
fi

[ $? -eq 0 ] && exit 1

exit 0
