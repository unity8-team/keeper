import dbus
import time
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

BUS_NAME = 'com.canonical.keeper'
MAIN_OBJ = '/com/canonical/keeper'
MAIN_IFACE = 'com.canonical.keeper'
SYSTEM_BUS = False

STORE_PATH_PREFIX = MAIN_OBJ
STORE_IFACE = 'com.canonical.pay.store'

ERR_PREFIX = 'org.freedesktop.DBus.Error'
ERR_INVAL = ERR_PREFIX + '.InvalidArgs'
ERR_ACCESS = ERR_PREFIX + '.AccessDenied'


#
# Util
#

def encode_path_element(element):
    encoded = []
    first = True
    for ch in element:
        if (ch.isalpha() or (ch.isdigit() and not first)):
            encoded.append(ch)
        else:
            encoded.append('_{:02x}'.format(ord(ch)))
    return ''.join(encoded)


def build_store_path(package_name):
    return STORE_PATH_PREFIX + '/' + encode_path_element(package_name)

#
#  Store
#


class Item:
    __default_bus_properties = {
        'acknowledged_timestamp': dbus.UInt64(0.0),
        'completed_timestamp': dbus.UInt64(0.0),
        'description': dbus.String('The is a default item'),
        'price': dbus.String('$1'),
        'purchase_id': dbus.UInt64(0.0),
        'refundable_until': dbus.UInt64(0.0),
        'sku': dbus.String('default_item'),
        'state': dbus.String('available'),
        'type': dbus.String('unlockable'),
        'title': dbus.String('Default Item')
    }

    def __init__(self, sku):
        self.bus_properties = Item.__default_bus_properties.copy()
        self.bus_properties['sku'] = sku

    def serialize(self):
        return dbus.Dictionary(self.bus_properties)

    def set_property(self, key, value):
        if key not in self.bus_properties:
            raise dbus.exceptions.DBusException(
                ERR_INVAL,
                'Invalid item property {0}'.format(key))
        self.bus_properties[key] = value


def store_add_item(store, properties):
    if 'sku' not in properties:
        raise dbus.exceptions.DBusException(
            ERR_INVAL,
            'item has no sku property')

    sku = properties['sku']
    if sku in store.items:
        raise dbus.exceptions.DBusException(
            ERR_INVAL,
            'store {0} already has item {1}'.format(store.name, sku))

    item = Item(sku)
    store.items[sku] = item
    store.set_item(store, sku, properties)


def store_set_item(store, sku, properties):
    try:
        item = store.items[sku]
        for key, value in properties.items():
            item.set_property(key, value)
    except KeyError:
        raise dbus.exceptions.DBusException(
            ERR_INVAL,
            'store {0} has no such item {1}'.format(store.name, sku))


def store_get_item(store, sku):
    try:
        return store.items[sku].serialize()
    except KeyError:
        if store.path.endswith(encode_path_element('click-scope')):
            return dbus.Dictionary(
                {
                    'sku': sku,
                    'state': 'available',
                    'refundable_until': 0,
                })
        else:
            raise dbus.exceptions.DBusException(
                ERR_INVAL,
                'store {0} has no such item {1}'.format(store.name, sku))


def store_get_purchased_items(store):
    items = []
    for sku, item in store.items.items():
        if item.bus_properties['state'] in ('approved', 'purchased'):
            items.append(item.serialize())
    return dbus.Array(items, signature='a{sv}', variant_level=1)


def store_purchase_item(store, sku):
    if store.path.endswith(encode_path_element('click-scope')):
        if sku != 'cancel':
            item = Item(sku)
            item.bus_properties = {
                'state': 'purchased',
                'refundable_until': dbus.UInt64(time.time() + 15*60),
                'sku': sku,
            }
            store.items[sku] = item

        return store_get_item(store, sku)
    try:
        if sku == 'denied':
            raise dbus.exceptions.DBusException(
                ERR_ACCESS,
                'User denied access.')
        elif sku != 'cancel':
            item = store.items[sku]
            item.set_property('state', 'approved')
            item.set_property('purchase_id',
                              dbus.UInt64(store.next_purchase_id))
            item.set_property('completed_timestamp', dbus.UInt64(time.time()))
            store.next_purchase_id += 1
        return store_get_item(store, sku)
    except KeyError:
        raise dbus.exceptions.DBusException(
            ERR_INVAL,
            'store {0} has no such item {1}'.format(store.name, sku))


def store_refund_item(store, sku):
    if not store.path.endswith(encode_path_element('click-scope')):
        raise dbus.exceptions.DBusException(
            ERR_INVAL,
            'Refunds are only available for packages')

    try:
        item = store.items[sku]
        if (item.bus_properties['state'] == 'purchased' and
            (item.bus_properties['refundable_until'] >
             dbus.UInt64(time.time()))):
            del store.items[sku]
            return dbus.Dictionary({
                'state': 'available',
                'sku': sku,
                'refundable_until': 0
            })
        else:
            return dbus.Dictionary({
                'state': 'purchased',
                'sku': sku,
                'refundable_until': 0
            })
    except KeyError:
        return dbus.Dictionary({'state': 'available', 'sku': sku,
                                'refundable_until': 0})


def store_acknowledge_item(store, sku):
    if store.path.endswith(encode_path_element('click-scope')):
        raise dbus.exceptions.DBusException(
            ERR_INVAL,
            'Only in-app purchase items can be acknowledged')

    try:
        item = store.items[sku]
        item.set_property('acknowledged_timestamp', dbus.UInt64(time.time()))
        item.set_property('state', dbus.String('purchased'))
        return item.serialize()
    except KeyError:
        raise dbus.exceptions.DBusException(
            ERR_INVAL,
            'store {0} has no such item {1}'.format(store.name, sku))

#
#  Main 'Storemock' Obj
#


def main_add_store(mock, package_name, items):
    path = build_store_path(package_name)
    mock.AddObject(path, STORE_IFACE, {}, [])
    store = mockobject.objects[path]
    store.name = package_name
    store.items = {}
    store.next_purchase_id = 1

    store.add_item = store_add_item
    store.set_item = store_set_item
    store.get_item = store_get_item
    store.get_purchased_items = store_get_purchased_items
    store.purchase_item = store_purchase_item
    store.refund_item = store_refund_item
    store.acknowledge_item = store_acknowledge_item
    store.AddMethods(STORE_IFACE, [
        ('AddItem', 'a{sv}', '',
         'self.add_item(self, args[0])'),
        ('SetItem', 'sa{sv}', '',
         'self.set_item(self, args[0], args[1])'),
        ('GetItem', 's', 'a{sv}',
         'ret = self.get_item(self, args[0])'),
        ('GetPurchasedItems', '', 'aa{sv}',
         'ret = self.get_purchased_items(self)'),
        ('PurchaseItem', 's', 'a{sv}',
         'ret = self.purchase_item(self, args[0])'),
        ('RefundItem', 's', 'a{sv}',
         'ret = self.refund_item(self, args[0])'),
        ('AcknowledgeItem', 's', 'a{sv}',
         'ret = self.acknowledge_item(self, args[0])'),
    ])

    for item in items:
        store.add_item(store, item)


def main_get_stores(mock):
    names = []
    for key, val in mockobject.objects.items():
        try:
            names.append(val.name)
        except AttributeError:
            pass
    return dbus.Array(names, signature='s', variant_level=1)


def main_add_item(mock, package_name, item):
    try:
        path = build_store_path(package_name)
        # mock.log('store {0} adding item {1}'.format(path, item))
        mockobject.objects[path].StoreAddItem(item)
    except KeyError:
        raise dbus.exceptions.DBusException(
            ERR_INVAL,
            'no such package {0}'.format(package_name))


def main_set_item(mock, package_name, item):
    try:
        path = build_store_path(package_name)
        store = mock.objects[path]
        store.set_item(store, item)
    except KeyError:
        raise dbus.exceptions.DBusException(
            ERR_INVAL,
            'error adding item to package {0}'.format(package_name))


def load(main, parameters):

    main.add_store = main_add_store
    main.add_item = main_add_item
    main.set_item = main_set_item
    main.get_stores = main_get_stores
    main.AddMethods(MAIN_IFACE, [
        ('AddStore', 'saa{sv}', '', 'self.add_store(self, args[0], args[1])'),
        ('AddItem', 'sa{sv}', '', 'self.add_item(self, args[0], args[1])'),
        ('SetItem', 'sa{sv}', '', 'self.set_item(self, args[0], args[1])'),
        ('GetStores', '', 'as', 'ret = self.get_stores(self)'),
    ])
