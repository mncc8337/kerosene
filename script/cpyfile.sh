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

if [ -z "$(losetup -a | grep "/dev/loop1")" ]; then
    sudo losetup /dev/loop1 disk.img -o 1048576 # 1024^2
fi
sudo mount --onlyonce /dev/loop1 ./mnt

if sudo cp -r --preserve=timestamps $1 $dest; then
    echo copied $1 to $dest
fi

sudo umount ./mnt
sudo losetup -d /dev/loop1

sync
