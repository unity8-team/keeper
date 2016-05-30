#! /usr/bin/env python3

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
# Little helper program to test that public header files don't include internal header files.
#
# Usage: check_public_headers.py directory
#
# The directory specifies the location of the header files. All files in that directory ending in .h (but not
# in subdirectories) are tested.
#

import argparse
import os
import sys
import re

#
# Write the supplied message to stderr, preceded by the program name.
#
def error(msg):
    print(os.path.basename(sys.argv[0]) + ": " + msg, file=sys.stderr)

#
# Write the supplied message to stdout, preceded by the program name.
#
def message(msg):
    print(os.path.basename(sys.argv[0]) + ": " + msg)

#
# For each of the supplied headers, check whether that header includes something in an internal directory.
# Return the count of headers that do this.
#
def test_files(hdr_dir, hdrs):
    num_errs = 0
    for hdr in hdrs:
        try:
            hdr_name = os.path.join(hdr_dir, hdr)
            file = open(hdr_name, 'r', encoding = 'utf=8')
        except OSError as e:
            error("cannot open \"" + hdr_name + "\": " + e.strerror)
            sys.exit(1)

        include_pat = re.compile(r'#[ \t]*include[ \t]+[<"](.*?)[>"]')

        lines = file.readlines()
        line_num = 0
        for l in lines:
            line_num += 1
            include_mo = include_pat.match(l)
            if include_mo:
                hdr_path = include_mo.group(1)
                if 'internal/' in hdr_path:
                    num_errs += 1
                    # Yes, write to stdout because this is expected output
                    message(hdr_name + " includes an internal header at line " + str(line_num) + ": " + hdr_path)
    return num_errs

def run():
    #
    # Parse arguments.
    #
    parser = argparse.ArgumentParser(description = 'Test that no public header includes an internal header.')
    parser.add_argument('dir', nargs = 1, help = 'The directory to look for header files ending in ".h"')
    args = parser.parse_args()

    #
    # Find all the .h files in specified directory and look for #include directives that mention "internal/".
    #
    hdr_dir = args.dir[0]
    try:
        files = os.listdir(hdr_dir)
    except OSError as e:
        error("cannot open \"" + hdr_dir + "\": " + e.strerror)
        sys.exit(1)
    hdrs = [hdr for hdr in files if hdr.endswith('.h')]

    if test_files(hdr_dir, hdrs) != 0:
        sys.exit(1)                     # Errors were reported earlier

if __name__ == '__main__':
   run()
