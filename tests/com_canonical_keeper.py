import copy
import dbus
import dbus.service
import os
import socket
import subprocess
import sys
import time
from dbusmock import OBJECT_MANAGER_IFACE, mockobject
from gi.repository import GLib

'''com.canonical.keeper mock template
'''

# (c) 2016 Canonical Ltd.
#
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
USER_PATH = '/'.join([SERVICE_PATH, 'user'])
USER_IFACE = '.'.join([SERVICE_IFACE, 'User'])
HELPER_PATH = '/'.join([SERVICE_PATH, 'helper'])
HELPER_IFACE = '.'.join([SERVICE_IFACE, 'Helper'])
MOCK_IFACE = '.'.join([SERVICE_IFACE, 'Mock'])

ACTION_CANCELLED = 'cancelled'
ACTION_COMPLETE = 'complete'
ACTION_FAILED = 'failed'
ACTION_RESTORING = 'restoring'
ACTION_SAVING = 'saving'
ACTION_QUEUED = 'queued'

KEY_ACTION = 'action'
KEY_BLOB = 'blob-data'
KEY_CTIME = 'ctime'
KEY_ERROR = 'error'
KEY_HELPER = 'helper-exec'
KEY_NAME = 'display-name'
KEY_PERCENT_DONE = 'percent-done'
KEY_SIZE = 'size'
KEY_SPEED = 'speed'
KEY_SUBTYPE = 'subtype'
KEY_TYPE = 'type'
KEY_UUID = 'uuid'

TYPE_APP = 'application'
TYPE_FOLDER = 'folder'
TYPE_SYSTEM = 'system-data'

# magic keys used by dbusmock
BUS_NAME = 'com.canonical.keeper'
MAIN_IFACE = SERVICE_IFACE
MAIN_OBJ = SERVICE_PATH
SYSTEM_BUS = False


#
#  classes
#


class TaskData:
    def __init__(self):
        self.action = ACTION_QUEUED
        self.blob = None
        self.n_bytes = 0
        self.n_left = 0
        self.error = ''
        self.chunks = []
        self.sock = None
        self.uuid = None
        self.bytes_per_second = {}


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
    if user.remaining_tasks:
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
        user.log('last task finished; setting user.current_task to None')
        user.current_task = None
        user.update_state_property(user)

    else:
        uuid = user.remaining_tasks.pop(0)
        user.current_task = uuid

        # update the action state
        if user.backup_choices.get(uuid):
            action = ACTION_SAVING
        else:
            action = ACTION_RESTORING
        user.task_data[uuid].action = action
        user.update_state_property(user)

        # find the helper to run
        choice = user.backup_choices.get(uuid)
        if not choice:
            choice = user.restore_choices.get(uuid)
        helper_exec = choice.get(KEY_HELPER)

        # build the helper's environment variables
        henv = {}
        henv['QDBUS_DEBUG'] = '1'
        henv['G_DBUS_DEBUG'] = 'call,message,signal,return'
        for key in ['DBUS_SESSION_BUS_ADDRESS', 'DBUS_SYSTEM_BUS_ADDRESS']:
            val = os.environ.get(key, None)
            if val:
                henv[key] = val

        # set the helper's cwd
        if choice.get(KEY_TYPE) == TYPE_FOLDER:
            helper_cwd = choice.get(KEY_SUBTYPE)
        else:
            # FIXME: other types
            helper_cwd = os.getcwd()

        # spawn the helper
        user.log('starting %s for %s, env %s' % (helper_exec, uuid, henv))
        user.process = subprocess.Popen(
            [helper_exec, HELPER_PATH],
            env=henv, stdout=sys.stdout, stderr=sys.stderr,
            cwd=helper_cwd,
            shell=helper_exec.endswith('.sh')
        )

        GLib.timeout_add(10, user.periodic_func, user)


def user_periodic_func(user):

    done = False
    got_data_this_pass = False

    if not user.process:
        fail("bug: user_process_check_func called w/o user.process")

    uuid = user.current_task
    td = user.task_data[uuid]

    # did the helper exit with an error code?
    returncode = user.process.poll()
    if returncode:
        error = 'helper exited with a returncode of %s' % (str(returncode))
        user.log(error)
        td.error = error
        td.action = ACTION_FAILED
        done = True

    # try to read the socket
    if td.sock and not td.error:
        chunk = td.sock.recv(4096*2)
        chunk_len = len(chunk)
        if chunk_len:
            got_data_this_pass = True
            td.chunks.append(chunk)
            td.n_left -= chunk_len
            user.log('got %s more bytes; %s left' % (chunk_len, td.n_left))
        if td.n_left <= 0:
            done = True

    # if done, clean up the socket
    if done and td.sock:
        user.log('cleaning up sock')
        td.sock.shutdown(socket.SHUT_RDWR)
        td.sock.close()
        td.sock = None

    # if done successfully, save the blob
    if done and not td.error:
        user.log('setting blob')
        blob = b''.join(td.chunks)
        td.blob = blob
        user.log('backup %s done; %s bytes' % (uuid, len(blob)))
        td.action = ACTION_COMPLETE

    # maybe update the task's state
    if done or got_data_this_pass:
        user.update_state_property(user)

    if done:
        user.process = None
        user.start_next_task(user)

    return not done


def user_init_tasks(user, uuids):
    user.all_tasks = uuids
    user.remaining_tasks = copy.copy(uuids)
    tds = {}
    for uuid in uuids:
        td = TaskData()
        td.uuid = uuid
        tds[uuid] = td
    user.task_data = tds


def user_start_backup(user, uuids):

    # sanity checks
    fail_if_busy()
    for uuid in uuids:
        if uuid not in user.backup_choices:
            badarg('uuid %s is not a valid backup choice' % (uuid))

    user.init_tasks(user, uuids)
    user.start_next_task(user)


def user_start_restore(user, uuids):

    # sanity checks
    fail_if_busy()
    for uuid in uuids:
        if uuid not in user.restore_choices:
            badarg('uuid %s is not a valid restore choice' % (uuid))

    user.init_tasks(user, uuids)
    user.start_next_task(user)


def user_cancel(user):
    # FIXME
    pass


def user_build_state(user):
    """Returns a generated state dictionary.

    State is a map of opaque backup keys to property maps.
    See the documentation for com.canonical.keeper.User's
    State() method for more information.
    """

    tasks_states = {}
    for uuid in user.all_tasks:
        task_state = {}

        # action
        td = user.task_data[uuid]
        action = td.action
        task_state[KEY_ACTION] = dbus.String(action)

        # display-name
        choice = user.backup_choices.get(uuid, None)
        if not choice:
            choice = user.restore_choices.get(uuid, None)
        if not choice:
            fail("couldn't find a choice for uuid %s" % (uuid))
        display_name = choice.get(KEY_NAME, None)
        task_state[KEY_NAME] = dbus.String(display_name)

        # error
        if action == ACTION_FAILED:
            task_state[KEY_ERROR] = dbus.String(td.error)

        # percent-done
        if td.n_bytes:
            p = dbus.Double((td.n_bytes - td.n_left) / td.n_bytes)
        else:
            p = dbus.Double(0.0)
        task_state[KEY_PERCENT_DONE] = p

        # speed
        helper = mockobject.objects[HELPER_PATH]
        if uuid == user.current_task:
            n_secs = 2
            n_bytes = 0
            too_old = time.time() - n_secs
            for key in td.bytes_per_second:
                helper.log('key is %s' % (str(key)))
                if key > too_old:
                    n_bytes += td.bytes_per_second[key]
            helper.log('n_bytes is %s' % (str(n_bytes)))
            bytes_per_second = n_bytes / n_secs
        else:
            bytes_per_second = 0
        task_state[KEY_SPEED] = dbus.Int32(bytes_per_second)

        # uuid
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


def helper_start_backup(helper, n_bytes):

    # are we forcing a fail?
    main = mockobject.objects[SERVICE_PATH]
    if main.fail_next_helper_start:
        main.fail_next_helper_start = False
        fail('main.fail_next_helper_start was set')

    helper.log("got start_backup request for %s bytes" % (n_bytes))

    parent, child = socket.socketpair()

    user = mockobject.objects[USER_PATH]
    uuid = user.current_task

    td = user.task_data[uuid]
    td.n_bytes = n_bytes
    td.n_left = n_bytes
    td.sock = parent

    return dbus.types.UnixFd(child.fileno())


def helper_start_restore(helper):
    helper.parent, child = socket.socketpair()
    return child


#
#  Controlling the mock
#


def mock_add_backup_choice(mock, uuid, props):

    keys = [KEY_NAME, KEY_TYPE, KEY_SUBTYPE, KEY_HELPER]
    if set(keys) != set(props.keys()):
        badarg('need props: %s got %s' % (keys, props.keys()))

    user = mockobject.objects[USER_PATH]
    user.backup_choices[uuid] = dbus.Dictionary(
        props,
        signature='sv',
        variant_level=1
    )


def mock_add_restore_choice(mock, uuid, props):

    keys = [KEY_NAME, KEY_TYPE, KEY_SUBTYPE, KEY_HELPER,
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
    user = mockobject.objects[USER_PATH]
    td = user.task_data[uuid]
    user.log('returning %s byte blob for uuid %s' % (len(td.blob), uuid))
    return td.blob


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
    o.init_tasks = user_init_tasks
    o.start_backup = user_start_backup
    o.get_restore_choices = user_get_restore_choices
    o.start_restore = user_start_restore
    o.cancel = user_cancel
    o.build_state = user_build_state
    o.update_state_property = user_update_state_property
    o.start_next_task = user_start_next_task
    o.all_tasks = []
    o.remaining_tasks = []
    o.task_data = {}
    o.backup_data = {}
    o.backup_choices = parameters.get('backup-choices', {})
    o.restore_choices = parameters.get('restore-choices', {})
    o.current_task = None
    o.process = None
    o.periodic_func = user_periodic_func
    o.defined_types = [TYPE_APP, TYPE_SYSTEM, TYPE_FOLDER]
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
    o.fail_next_helper_start = False
    o.AddMethods(MOCK_IFACE, [
        ('AddBackupChoice', 'sa{sv}', '',
         'self.add_backup_choice(self, args[0], args[1])'),
        ('AddRestoreChoice', 'sa{sv}', '',
         'self.add_restore_choice(self, args[0], args[1])'),
        ('GetBackupData', 's', 'ay',
         'ret = self.get_backup_data(self, args[0])'),
        ('FailNextHelperStart', '', '',
         'self.fail_next_helper_start = True'),
    ])
    o.EmitSignal(
        OBJECT_MANAGER_IFACE,
        'InterfacesAdded',
        'oa{sa{sv}}',
        [SERVICE_PATH, {MOCK_IFACE: {}}]
    )
