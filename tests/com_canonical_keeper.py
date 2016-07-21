import copy
import dbus
import dbus.service
import os
import socket
import subprocess
import sys
from dbusmock import OBJECT_MANAGER_IFACE, mockobject
from gi.repository import GLib

'''com.canonical.keeper mock template
'''

# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.  See http://www.gnu.org/copyleft/lgpl.html for the full text
# of the license.

__author__ = 'Charles Kerr'
__email__ = 'charles.kerr@canonical.com'
__copyright__ = '(c) 2016 Canonical Ltd.'
__license__ = 'LGPL 3+'

SERVICE_PATH = '/com/canonical/keeper'
SERVICE_IFACE = 'com.canonical.keeper'
USER_PATH = '/com/canonical/keeper/user'
USER_IFACE = 'com.canonical.keeper.User'
HELPER_PATH = '/com/canonical/keeper/helper'
HELPER_IFACE = 'com.canonical.keeper.Helper'
MOCK_IFACE = 'com.canonical.keeper.Mock'

# magic keys used by dbusmock
BUS_NAME = 'com.canonical.keeper'
MAIN_IFACE = SERVICE_IFACE
MAIN_OBJ = SERVICE_PATH
SYSTEM_BUS = False

ACTION_QUEUED = 0
ACTION_SAVING = 1
ACTION_RESTORING = 2
ACTION_COMPLETE = 3
ACTION_STOPPED = 4

KEY_CTIME = 'ctime'
KEY_BLOB = 'blob-data'
KEY_HELPER = 'helper-exec'
KEY_ICON = 'icon'
KEY_NAME = 'display-name'
KEY_SIZE = 'size'
KEY_SUBTYPE = 'subtype'
KEY_TYPE = 'type'
KEY_UUID = 'uuid'

#
#  Utils
#


def badarg(msg):
    raise dbus.exceptions.DBusException(
        msg,
        name='org.freedesktop.DBus.Error.InvalidArgs'
    )


def fail(msg):
    raise dbus.exceptions.DBusException(
        msg,
        name='org.freedesktop.DBus.Error.Failed'
    )


def fail_if_busy():
    user = mockobject.objects[USER_PATH]
    if user.all_tasks:
        fail("can't do that while service is busy")


#
#  User Obj
#


def user_get_backup_choices(user):
    return dbus.Dictionary(
        user.backup_choices,
        signature='sa{sv}',
        variant_level=1
    )


def user_get_restore_choices(user):
    return dbus.Dictionary(
        user.restore_choices,
        signature='sa{sv}',
        variant_level=1
    )


def user_start_next_task(user):
    if not user.remaining_tasks:
        user.log('last task finished')
        user.all_tasks = []
        user.current_task = None
        user.log('setting user.current_task to None')
        user.update_state_property(user)

    else:
        uuid = user.remaining_tasks.pop(0)
        user.current_task = uuid
        user.update_state_property(user)

        # find the helper to run
        choice = user.backup_choices.get(uuid)
        if not choice:
            choice = user.restore_choices.get(uuid)
        helper_exec = choice.get(KEY_HELPER)

        # build the env that we'll pass to the helper
        henv = {}
        henv['QDBUS_DEBUG'] = '1'
        henv['G_DBUS_DEBUG'] = 'call,message,signal,return'
        for key in ['DBUS_SESSION_BUS_ADDRESS', 'DBUS_SYSTEM_BUS_ADDRESS']:
            val = os.environ.get(key, None)
            if val:
                henv[key] = val
        # FIXME: add PWD

        # spawn the helper
        user.log('starting %s for %s, env %s' % (helper_exec, uuid, henv))
        subprocess.Popen(
            [helper_exec, HELPER_PATH],
            env=henv, stdout=sys.stdout, stderr=sys.stderr
        )


def user_start_backup(user, uuids):

    # sanity checks
    fail_if_busy()
    for uuid in uuids:
        if uuid not in user.backup_choices:
            badarg('uuid %s is not a valid backup choice' % (uuid))

    user.all_tasks = uuids
    user.remaining_tasks = copy.copy(uuids)
    user.start_next_task(user)


def user_start_restore(user, uuids):

    # sanity checks
    fail_if_busy()
    for uuid in uuids:
        if uuid not in user.restore_choices:
            badarg('uuid %s is not a valid backup choice' % (uuid))

    user.all_tasks = uuids
    user.remaining_tasks = copy.copy(uuids)
    user.start_next_task()


def user_cancel(user):
    # FIXME
    pass


def user_build_state(user):
    """Returns a generated state dictionary.

    State is a map of opaque backup keys to property maps,
    which will contain a 'display-name' string and 'action' int
    whose possible values are queued(0), saving(1),
    restoring(2), complete(3), and stopped(4).

    Some property maps may also have an 'item' string
    and a 'percent-done' double [0..1.0].
    For example if these are set to (1, "Photos", 0.36),
    the user interface could show "Backing up Photos (36%)".
    Clients should gracefully handle missing properties;
    eg a missing percent-done could instead show
    "Backing up Photos".

    A failed task's property map may also contain an 'error'
    string if set by the backup helpers.
    """

    tasks_states = {}
    for uuid in user.all_tasks:
        task_state = {}

        # get the task's action
        if uuid == user.current_task:
            if uuid in user.backup_choices:
                action = ACTION_SAVING
            else:
                action = ACTION_RESTORING
        elif uuid in user.remaining_tasks:
            action = ACTION_QUEUED
        else:  # FIXME: 'ACTION_STOPPED' not handled yet
            action = ACTION_COMPLETE
        task_state['action'] = dbus.Int32(action)

        # get the task's display-name
        choice = user.backup_choices.get(uuid, None)
        if not choice:
            choice = user.restore_choices.get(uuid, None)
        if not choice:
            fail("couldn't find a choice for uuid %s" % (uuid))
        display_name = choice.get(KEY_NAME, None)
        task_state[KEY_NAME] = dbus.String(display_name)

        # FIXME: use a real percentage here
        if action == ACTION_COMPLETE:
            percent_done = dbus.Double(1.0)
        elif action == ACTION_SAVING or action == ACTION_RESTORING:
            percent_done = dbus.Double(0.5)
        else:
            percent_done = dbus.Double(0.0)
        task_state['percent-done'] = percent_done

        tasks_states[uuid] = dbus.Dictionary(task_state)

    return dbus.Dictionary(
        tasks_states,
        signature='sa{sv}',
        variant_level=1
    )


def user_update_state_property(user):
    old_state = user.Get(USER_IFACE, 'State')
    new_state = user.build_state(user)
    if old_state != new_state:
        user.Set(USER_IFACE, 'State', new_state)


#
#  Helper Obj
#

class HelperWork:
    chunks = None
    n_bytes = None
    n_left = None
    sock = None
    uuid = None


def helper_periodic_func(helper):

    if not helper.work:
        fail("bug: helper_periodic_func called w/o helper.work")

    # try to read a bit
    chunk = helper.work.sock.recv(4096)
    if len(chunk):
        helper.work.chunks.append(chunk)
        helper.work.n_left -= len(chunk)
        helper.log('got %s bytes; %s left' % (len(chunk), helper.work.n_left))

    # cleanup if done
    done = helper.work.n_left <= 0
    if done:
        helper.work.sock.shutdown(socket.SHUT_RDWR)
        helper.work.sock.close()
        user = mockobject.objects[USER_PATH]
        user.backup_data[helper.work.uuid] = b''.join(helper.work.chunks)
        user.log(
            'backup %s done; %s bytes' %
            (helper.work.uuid, len(user.backup_data[helper.work.uuid]))
        )
        user.start_next_task(user)
        helper.work = None

    if len(chunk) or done:
        user = mockobject.objects[USER_PATH]
        user.update_state_property(user)

    return not done


def helper_start_backup(helper, n_bytes):

    helper.log("got start_backup request for %s bytes" % (n_bytes))
    if helper.work:
        fail("can't start a new backup while one's already active")

    parent, child = socket.socketpair()

    # set up helper's workarea
    work = HelperWork()
    work.chunks = []
    work.n_bytes = n_bytes
    work.n_left = n_bytes
    work.sock = parent
    work.uuid = mockobject.objects[USER_PATH].current_task
    helper.work = work

    # start checking periodically
    GLib.timeout_add(10, helper.periodic_func, helper)
    return dbus.types.UnixFd(child.fileno())


def helper_start_restore(helper):
    helper.parent, child = socket.socketpair()
    return child


#
#  Controlling the mock
#


def mock_add_backup_choice(mock, uuid, props):

    keys = [KEY_NAME, KEY_TYPE, KEY_SUBTYPE, KEY_ICON, KEY_HELPER]
    if set(keys) != set(props.keys()):
        badarg('need props: %s got %s' % (keys, props.keys()))

    user = mockobject.objects[USER_PATH]
    user.backup_choices[uuid] = dbus.Dictionary(
        props,
        signature='sv',
        variant_level=1
    )


def mock_add_restore_choice(mock, uuid, props):

    keys = [KEY_NAME, KEY_TYPE, KEY_SUBTYPE, KEY_ICON, KEY_HELPER,
            KEY_SIZE, KEY_CTIME, KEY_BLOB]
    if set(keys) != set(props.keys()):
        badarg('need props: %s got %s' % (keys, props.keys()))

    user = mockobject.objects[USER_PATH]
    user.restore_choices[uuid] = dbus.Dictionary(
        props,
        signature='sv',
        variant_level=1
    )


def mock_get_backup_data(mock, uuid):
    blob = mockobject.objects[USER_PATH].backup_data[uuid]
    mock.log('returning %s byte blob for uuid %s' % (len(blob), uuid))
    return blob


#
#
#

def load(main, parameters):

    main.log('Keeper template paramers: "' + str(parameters) + '"')

    # com.canonical.keeper.User
    path = USER_PATH
    main.AddObject(path, USER_IFACE, {}, [])
    o = mockobject.objects[path]
    o.get_backup_choices = user_get_backup_choices
    o.start_backup = user_start_backup
    o.get_restore_choices = user_get_restore_choices
    o.start_restore = user_start_restore
    o.cancel = user_cancel
    o.build_state = user_build_state
    o.update_state_property = user_update_state_property
    o.start_next_task = user_start_next_task
    o.all_tasks = []
    o.remaining_tasks = []
    o.backup_data = {}
    o.backup_choices = parameters.get('backup-choices', {})
    o.restore_choices = parameters.get('restore-choices', {})
    o.current_task = None
    o.defined_types = ['application', 'system-data', 'folder']
    o.AddMethods(USER_IFACE, [
        ('GetBackupChoices', '', 'a{sa{sv}}',
         'ret = self.get_backup_choices(self)'),
        ('StartBackup', 'as', '',
         'self.start_backup(self, args[0])'),
        ('GetRestoreChoices', '', 'a{sa{sv}}',
         'ret = self.get_restore_choices(self)'),
        ('StartRestore', 'as', '',
         'self.start_restore(self, args[0])'),
        ('Cancel', '', '',
         'self.cancel(self)'),
    ])
    o.AddProperty(USER_IFACE, "State", o.build_state(o))

    # com.canonical.keeper.Helper
    path = HELPER_PATH
    main.AddObject(path, HELPER_IFACE, {}, [])
    o = mockobject.objects[path]
    o.start_backup = helper_start_backup
    o.start_restore = helper_start_restore
    o.periodic_func = helper_periodic_func
    o.work = None
    o.AddMethods(HELPER_IFACE, [
        ('StartBackup', 't', 'h',
         'ret = self.start_backup(self, args[0])'),
        ('StartRestore', '', 'h',
         'ret = self.start_restore(self)')
    ])

    # com.canonical.keeper.Mock
    o = main
    o.add_backup_choice = mock_add_backup_choice
    o.add_restore_choice = mock_add_restore_choice
    o.get_backup_data = mock_get_backup_data
    o.AddMethods(MOCK_IFACE, [
        ('AddBackupChoice', 'sa{sv}', '',
         'self.add_backup_choice(self, args[0], args[1])'),
        ('AddRestoreChoice', 'sa{sv}', '',
         'self.add_restore_choice(self, args[0], args[1])'),
        ('GetBackupData', 's', 'ay',
         'ret = self.get_backup_data(self, args[0])')
    ])
    o.EmitSignal(
        OBJECT_MANAGER_IFACE,
        'InterfacesAdded',
        'oa{sa{sv}}',
        [SERVICE_PATH, {MOCK_IFACE: {}}]
    )
