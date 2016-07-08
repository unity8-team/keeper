import copy
import dbus
import dbus.service
import os
import select
import socket
import subprocess
import sys
import threading
from dbusmock import mockobject

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


def raise_not_supported_if_active():
    user = mockobject.objects[USER_PATH]
    if user.is_active(user):
        raise dbus.exceptions.DBusException(
            "can't do that while service is active",
            name='org.freedesktop.DBus.Error.NotSupported'
        )

#
#  User Obj
#


def user_is_active(user):
    return len(user.all_tasks) != 0


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


def user_task_worker(user, helper_exec, my_env):
    # FIXME: threading issues abound
    user.log('starting "%s" with environment "%s"' % (helper_exec, my_env))
    p = subprocess.Popen(
        [helper_exec, HELPER_PATH],
        env=my_env, stdout=sys.stdout, stderr=sys.stderr
    )
    p.wait()
    user.log('process is done')
    user.start_next_task(user)


def user_start_next_task(user):
    if not len(user.remaining_tasks):
        user.log('last task finished')
        user.current_task = None
        user.log('setting user.current_task to None')
        user.update_state_property(user)
    else:
        uuid = user.remaining_tasks.pop(0)
        user.log('setting user.current_task to %s' % (uuid))
        user.current_task = uuid
        user.update_state_property(user)

        # kick of the helper executable in a worker thread
        choice = user.backup_choices.get(uuid, None)
        if not choice:
            choice = user.restore_choices.get(uuid)
        choice_type = choice['type']
        helper_exec = user.helpers[choice_type]
        my_env = {}
        my_env['QDBUS_DEBUG'] = '1'
        for key in ['DBUS_SESSION_BUS_ADDRESS', 'DBUS_SYSTEM_BUS_ADDRESS']:
            user.log(key)
            val = os.environ.get(key, None)
            if val:
                my_env[key] = val
        # FIXME: set real metainfo in my_env
        thread = threading.Thread(
            target=user.task_worker,
            args=(user, helper_exec, my_env)
        )
        thread.start()


def user_start_backup(user, uuids):

    # sanity checks
    user.raise_not_supported_if_active()
    for uuid in uuids:
        if uuid not in user.backup_choices:
            badarg('uuid %s is not a valid backup choice' % (uuid))

    user.all_tasks = uuids
    user.remaining_tasks = copy.copy(uuids)
    user.start_next_task(user)


def user_start_restore(user, uuids):

    # sanity checks
    user.raise_not_supported_if_active()
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
        display_name = user.backup_choices.get('display-name', None)
        if not display_name:
            display_name = user.restore_choices.get('display-name', None)
        if not display_name:
            display_name = 'Cynthia Rose'
        task_state['display-name'] = dbus.String(display_name)

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
    user.Set(USER_IFACE, 'State', user.build_state(user))


#
#  Helper Obj
#


def helper_backup_worker(helper, uuid, sock, n_bytes):
    n_read = 0
    chunks = []
    user = mockobject.objects[USER_PATH]

    while n_read < n_bytes:
        inputs = [sock]
        helper.log('waiting for helper to write to us')
        readable, writable, exceptional = select.select(inputs, [], inputs)
        helper.log(
            'len(r) %s len(w) %s len(x) %s' %
            (len(readable), len(writable), len(exceptional))
        )
        for s in readable:
            chunk = s.recv(4096)
            helper.log('recv got %s bytes' % (len(chunk)))
            if len(chunk):
                helper.log('received "%s"' % (chunk))
                chunks.append(chunk)
                n_read += len(chunk)
                # FIXME: threading issues here in update_state
                user.update_state_property(user)

    sock.shutdown(socket.SHUT_RDWR)
    sock.close()

    user = mockobject.objects[USER_PATH]
    user.backup_data[uuid] = b''.join(chunks)
    user.log('backup %s done; %s bytes' % (uuid, len(user.backup_data[uuid])))
    user.update_state_property(user)


def helper_start_backup(helper, n_bytes):
    helper.log("got start_backup request for %s bytes" % (n_bytes))
    parent, child = socket.socketpair()
    user = mockobject.objects[USER_PATH]
    t = threading.Thread(
        target=helper.backup_worker,
        args=(helper, user.current_task, parent, n_bytes)
    )
    t.start()
    return dbus.types.UnixFd(child.fileno())


def helper_start_restore(helper):
    helper.parent, child = socket.socketpair()
    return child


#
#
#

def load(main, parameters):

    main.log('hello world. parameters is: "' + str(parameters) + '"')

    path = USER_PATH
    main.AddObject(path, USER_IFACE, {}, [])
    user = mockobject.objects[path]
    user.get_backup_choices = user_get_backup_choices
    user.start_backup = user_start_backup
    user.get_restore_choices = user_get_restore_choices
    user.start_restore = user_start_restore
    user.is_active = user_is_active
    user.cancel = user_cancel
    user.build_state = user_build_state
    user.update_state_property = user_update_state_property
    user.raise_not_supported_if_active = raise_not_supported_if_active
    user.start_next_task = user_start_next_task
    user.task_worker = user_task_worker
    user.badarg = badarg
    user.fail = fail
    user.all_tasks = []
    user.remaining_tasks = []
    user.backup_data = {}
    user.backup_choices = parameters.get('backup-choices', {})
    user.restore_choices = parameters.get('restore-choices', {})
    user.helpers = parameters.get('helpers', {})
    user.current_task = None
    user.defined_types = ['application', 'system-data', 'folder']
    user.AddMethods(USER_IFACE, [
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
    user.AddProperty(USER_IFACE, "State", user.build_state(user))

    path = HELPER_PATH
    main.AddObject(path, HELPER_IFACE, {}, [])
    helper = mockobject.objects[path]
    helper.start_backup = helper_start_backup
    helper.start_restore = helper_start_restore
    helper.backup_worker = helper_backup_worker
    helper.AddMethods(HELPER_IFACE, [
        ('StartBackup', 't', 'h',
         'ret = self.start_backup(self, args[0])'),
        ('StartRestore', '', 'h',
         'ret = self.start_restore(self)')
    ])
