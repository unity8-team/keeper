# Copyright © 2015 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License version 3,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authors:
#     Xavi Garcia <marcus.tomlinson@canonical.com>

# disable compilier warnings in machine-generated code
add_compile_options(-w)

# XML

set(
  properties_xml
  "org.freedesktop.DBus.Properties.xml"
)

set_source_files_properties(
    "${properties_xml}"
    PROPERTIES
    NO_NAMESPACE YES
    CLASSNAME DBusPropertiesInterface
)

qt5_add_dbus_interface(
  interface_files
  ${properties_xml}
  DBusPropertiesInterface
)

#
#

set(
  STUBS_LIB
  qdbus-stubs-tests
)

add_library(
  ${STUBS_LIB}
  STATIC
  ${interface_files}
)

