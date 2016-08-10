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
#     Marcus Tomlinson <marcus.tomlinson@canonical.com>

# disable compilier warnings in machine-generated code
add_compile_options(-w)

# XML

set(
  helper_xml
  "com.canonical.keeper.Helper.xml"
)

set(
  user_xml
  "com.canonical.keeper.User.xml"
)

set_source_files_properties(
    "${helper_xml}"
    "${user_xml}"
    PROPERTIES
    NO_NAMESPACE YES
    INCLUDE "qdbus-stubs/dbus-types.h"
)

# Helper Object

set_source_files_properties(
  ${helper_xml}
  PROPERTIES
  CLASSNAME DBusInterfaceKeeperHelper
)

qt5_add_dbus_interface(
  interface_files
  ${helper_xml}
  keeper_helper_interface
)

qt5_add_dbus_adaptor(
  adaptor_files
  ${helper_xml}
  ${CMAKE_SOURCE_DIR}/src/service/keeper-helper.h
  KeeperHelper
  KeeperHelperAdaptor
  KeeperHelperAdaptor
)

# User Object

set_source_files_properties(
  ${user_xml}
  PROPERTIES
  CLASSNAME DBusInterfaceKeeperUser
)

qt5_add_dbus_interface(
  interface_files
  ${user_xml}
  keeper_user_interface
)

qt5_add_dbus_adaptor(
  adaptor_files
  ${user_xml}
  ${CMAKE_SOURCE_DIR}/src/service/keeper-user.h
  KeeperUser
  KeeperUserAdaptor
  KeeperUserAdaptor
)

#
#

set(
  STUBS_LIB
  qdbus-stubs
)

add_library(
  ${STUBS_LIB}
  STATIC
  ${interface_files}
  ${adaptor_files}
)

target_link_libraries(
  ${STUBS_LIB}
  backup-helper
)
