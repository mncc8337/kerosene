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

sudo losetup /dev/loop0 disk.img -o 1048576 # 1024^2
sudo mount /dev/loop0 ./mnt

if sudo cp -r --preserve=timestamps $1 $dest; then
    echo copied $1 to $dest
fi

sudo umount ./mnt
sudo losetup -d /dev/loop0

sync
