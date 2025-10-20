#!/bin/sh
set -e

if [ -z "$1" ]; then
    echo no arg, quitting
    exit
fi

dest=".mnt/"

if [ -n "$2" ]; then
    dest="$2"
fi

if [ ! -d "$dest" ]; then
    mkdir -p $dest
fi

echo "setting up loop device..."
LOOP_DEV=$(sudo losetup --find --partscan ${BIN_DIR}disk.img --show)
if [ -z "$LOOP_DEV" ]; then
    echo "failed to set up loop device"
    exit 1
fi

echo "using loop device $LOOP_DEV"

sudo mount --onlyonce ${LOOP_DEV}p1 ./mnt

if sudo cp -r --preserve=timestamps $1 $dest; then
    echo copied $1 to $dest
fi

sudo umount ./mnt
sudo losetup -d $LOOP_DEV

sync
