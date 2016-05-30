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
# Little helper program to test that header files are stand-alone compilable (and therefore don't depend on
# other headers being included first).
#
# Usage: compile_headers.py directory compiler [compiler_flags]
#
# The directory specifies the location of the header files. All files in that directory ending in .h (but not
# in subdirectories) are tested.
#
# The compiler argument specifies the compiler to use (such as "gcc"), and the compiler_flags argument (which
# must be a single string argument, not a bunch of separate strings) specifies any additional flags, such
# as "-I -g". The flags need not include "-c".
#
# For each header file in the specified directory, the script create a corresponding .cpp that includes the
# header file. The .cpp file is created in the current directory (which isn't necessarily the same one as
# the directory the header files are in). The script runs the compiler on the generated .cpp file and, if the
# compiler returns non-zero exit status, it prints a message (on stdout) reporting the failure.
#
# The script does not stop if a file fails to compile. If all source files compile successfully, no output (other
# than the output from the compiler) is written, and the exit status is zero. If one or more files do not compile,
# or there are any other errors, such as not being able to open a file, exit status is non-zero.
#
# Messages about files that fail to compile are written to stdout. Message about other problems, such as non-existent
# files and the like, are written to stderr.
#
# The compiler's output goes to whatever stream the compiler writes to and is left alone.
#

import argparse
import os
import re
import shlex
import subprocess
import sys
import tempfile
import concurrent.futures, multiprocessing

# Additional #defines that should be set before including a header.
# This is intended for things such as enabling additional features
# that are conditionally compiled in the source.
extra_defines =[]

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
# Create a source file in the current directory that includes the specified header, compile it,
# and check exit status from the compiler. Throw if the compile command itself fails,
# return False if the compile command worked but reported errors, True if the compile succeeded.
#
def run_compiler(hdr, compiler, copts, verbose, hdr_dir):
    try:
        src = tempfile.NamedTemporaryFile(suffix='.cpp', dir='.')

        # Add any extra defines at the beginning of the temporary file.
        for flag in extra_defines:
            src.write(bytes("#define " + flag + "" + "\n", 'UTF-8'))

        src.write(bytes("#include <" + hdr + ">" + "\n", 'UTF-8'))
        src.flush()                                                 # Need this to make the file visible
        src_name = os.path.join('.', src.name)

        if verbose:
            print(compiler + " -c " + src_name + " " + copts)

        status = subprocess.call([compiler] + shlex.split(copts) + ["-c", src_name])
        if status != 0:
            message("cannot compile \"" + hdr + "\"") # Yes, write to stdout because this is expected output

        obj = os.path.splitext(src_name)[0] + ".o"
        try:
            os.unlink(obj)
        except:
            pass

        gcov = os.path.splitext(src_name)[0] + ".gcno"
        try:
            os.unlink(gcov)
        except:
            pass

        return status == 0

    except OSError as e:
        error(e.strerror)
        raise

#
# For each of the supplied headers, create a source file in the current directory that includes the header
# and then try to compile the header. Returns normally if all files could be compiled successfully and
# throws, otherwise.
#
def test_files(hdrs, compiler, copts, verbose, hdr_dir):
    num_errs = 0
    executor = concurrent.futures.ThreadPoolExecutor(max_workers=multiprocessing.cpu_count())
    futures = [executor.submit(run_compiler, h, compiler, copts, verbose, hdr_dir) for h in hdrs]
    for f in futures:
        try:
            if not f.result():
                num_errs += 1
        except OSError:
            num_errs += 1
            pass    # Error reported already
    if num_errs != 0:
        msg = str(num_errs) + " file"
        if num_errs != 1:
            msg += "s"
        msg += " failed to compile"
        message(msg)                    # Yes, write to stdout because this is expected output
        sys.exit(1)

def run():
    #
    # Parse arguments.
    #
    parser = argparse.ArgumentParser(description = 'Test that all headers in the passed directory compile stand-alone.')
    parser.add_argument('-v', '--verbose', action='store_true', help = 'Trace invocations of the compiler')
    parser.add_argument('dir', nargs = 1, help = 'The directory to look for header files ending in ".h"')
    parser.add_argument('compiler', nargs = 1, help = 'The compiler executable, such as "gcc"')
    parser.add_argument('copts', nargs = '?', default="",
                        help = 'The compiler options (excluding -c), such as "-g -Wall -I." as a single string.')
    args = parser.parse_args()

    #
    # Find all the .h files in specified directory and do the compilation for each one.
    #
    hdr_dir = args.dir[0]
    try:
        files = os.listdir(hdr_dir)
    except OSError as e:
        msg = "cannot open \"" + hdr_dir + "\": " + e.strerror
        error(msg)
        sys.exit(1)
    hdrs = [hdr for hdr in files if hdr.endswith('.h')]

    try:
        test_files(hdrs, args.compiler[0], args.copts, args.verbose, hdr_dir)
    except OSError:
        sys.exit(1)    # Errors were written earlier

if __name__ == '__main__':
    run()
