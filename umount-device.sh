#!/bin/sh

sudo umount ./mnt
sudo losetup -d /dev/loop0
# incase of error
sudo losetup -d /dev/loop1
