#!/bin/sh
set -e

sudo losetup /dev/loop0 disk.img -o 1048576 # 1024^2
sudo mount /dev/loop0 ./mnt

grub-mkrescue -o sos.iso --modules="normal part_msdos fat multiboot" ./mnt

sudo umount ./mnt
sudo losetup -d /dev/loop0

echo "iso generated"
