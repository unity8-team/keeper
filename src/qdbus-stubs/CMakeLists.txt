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


#set_source_files_properties(
#    "${DATA_DIR}/nm-device.xml"
#    "${DATA_DIR}/nm-device-wifi.xml"
#    "${DATA_DIR}/nm-access-point.xml"
#    "${DATA_DIR}/nm-active-connection.xml"
#    "${DATA_DIR}/nm-agent-manager.xml"
#    "${DATA_DIR}/nm-vpn-connection.xml"
#    "${DATA_DIR}/org.freedesktop.URfkill.xml"
#    "${DATA_DIR}/org.freedesktop.URfkill.Device.xml"
#    "${DATA_DIR}/org.freedesktop.URfkill.Killswitch.xml"
#    "${DATA_DIR}/com.canonical.powerd.xml"
#    PROPERTIES
#    NO_NAMESPACE YES
#)
#
#set_source_files_properties(
#    "${DATA_DIR}/nm-settings.xml"
#    "${DATA_DIR}/nm-settings-connection.xml"
#    "${DATA_DIR}/nm-manager.xml"
#    "${DATA_DIR}/org.freedesktop.Notifications.xml"
#    PROPERTIES
#    NO_NAMESPACE YES
#    INCLUDE "dbus-types.h"
#)
#
#qt5_add_dbus_interface(
#    CONNECTIVITY_BACKEND_SRC
#    "${DATA_DIR}/org.freedesktop.URfkill.xml"
#    URfkillInterface
#)
#
#qt5_add_dbus_interface(
#    CONNECTIVITY_BACKEND_SRC
#    "${DATA_DIR}/org.freedesktop.URfkill.Device.xml"
#    URfkillDeviceInterface
#)
#
#qt5_add_dbus_interface(
#    CONNECTIVITY_BACKEND_SRC
#    "${DATA_DIR}/org.freedesktop.URfkill.Killswitch.xml"
#    URfkillKillswitchInterface
#)
#
#qt5_add_dbus_interface(
#    CONNECTIVITY_BACKEND_SRC
#    "${DATA_DIR}/nm-manager.xml"
#    NetworkManagerInterface
#)
#
#qt5_add_dbus_interface(
#    CONNECTIVITY_BACKEND_SRC
#    "${DATA_DIR}/nm-device.xml"
#    NetworkManagerDeviceInterface
#)
#
#qt5_add_dbus_interface(
#    CONNECTIVITY_BACKEND_SRC
#    "${DATA_DIR}/nm-device-wifi.xml"
#    NetworkManagerDeviceWirelessInterface
#)
#
#qt5_add_dbus_interface(
#    CONNECTIVITY_BACKEND_SRC
#    "${DATA_DIR}/nm-access-point.xml"
#    NetworkManagerAccessPointInterface
#)
#
#qt5_add_dbus_interface(
#    CONNECTIVITY_BACKEND_SRC
#    "${DATA_DIR}/nm-active-connection.xml"
#    NetworkManagerActiveConnectionInterface
#)
#
#qt5_add_dbus_interface(
#    CONNECTIVITY_BACKEND_SRC
#    "${DATA_DIR}/nm-vpn-connection.xml"
#    NetworkManagerVpnConnectionInterface
#)
#
#qt5_add_dbus_interface(
#    CONNECTIVITY_BACKEND_SRC
#    "${DATA_DIR}/nm-settings-connection.xml"
#    NetworkManagerSettingsConnectionInterface
#)
#
#qt5_add_dbus_interface(
#    CONNECTIVITY_BACKEND_SRC
#    "${DATA_DIR}/nm-agent-manager.xml"
#    AgentManagerInterface
#)
#
#qt5_add_dbus_interface(
#    CONNECTIVITY_BACKEND_SRC
#    "${DATA_DIR}/nm-settings-connection.xml"
#    NetworkManagerSettingsConnectionInterface
#)
#
#qt5_add_dbus_interface(
#    CONNECTIVITY_BACKEND_SRC
#    "${DATA_DIR}/nm-settings.xml"
#    NetworkManagerSettingsInterface
#)
#
#qt5_add_dbus_interface(
#    CONNECTIVITY_BACKEND_SRC
#    "${DATA_DIR}/org.freedesktop.Notifications.xml"
#    NotificationsInterface
#)
#
#qt5_add_dbus_interface(
#    CONNECTIVITY_BACKEND_SRC
#    "${DATA_DIR}/com.canonical.powerd.xml"
#    PowerdInterface
#)
#
#add_library(
#    qdbus-stubs
#    STATIC
#    ${CONNECTIVITY_BACKEND_SRC}
#)
#
#qt5_use_modules(
#    qdbus-stubs
#    DBus
#)
