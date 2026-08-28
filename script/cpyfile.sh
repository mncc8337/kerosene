#!/bin/sh
set -e

if [ -z "$1" ]; then
    echo no arg, quitting
    exit
fi

dest="./mnt/"

if [ -n "$2" ]; then
    dest="$2"
fi

./script/mount-device.sh

if [ ! -d "$dest" ]; then
    mkdir -p $dest
fi

if cp -r --preserve=timestamps $1 $dest; then
    echo copied $1 to $dest
fi

./script/umount-device.sh

sync
