#!/bin/sh
set -e

if [ -z "$(losetup -a | grep "/dev/loop1")" ]; then
    sudo losetup /dev/loop1 ${BIN_DIR}disk.img -o 1048576 # 1024^2
fi
sudo mount --onlyonce /dev/loop1 ./mnt

grub-mkrescue -o kerosene.iso --modules="normal part_msdos fat multiboot" ./mnt

sudo umount ./mnt
sudo losetup -d /dev/loop1

echo "iso generated"
