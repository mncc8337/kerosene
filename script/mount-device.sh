#!/bin/sh
set -e

echo "setting up loop device..."
LOOP_DEV=$(sudo losetup --find --partscan ${BIN_DIR}disk.img --show)
if [ -z "$LOOP_DEV" ]; then
    echo "failed to set up loop device"
    exit 1
fi

echo "using loop device $LOOP_DEV"

sudo mount --onlyonce ${LOOP_DEV}p1 ./mnt

echo "mounted"
