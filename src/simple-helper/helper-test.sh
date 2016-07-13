#!/bin/bash

DBUS=${DBUS-PATH}
find src/helper/ -type f -print0 | ./src/tar/keeper-tar-create -0 -a /com/canonical/keeper/helper
sleep 30
touch /tmp/simple-helper-finished
