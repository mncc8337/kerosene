#!/bin/sh
set -e

make all
if [ ! -f disk.img ]; then
    ./gendiskimage.sh 65536
fi
./cpyfile.sh grub.cfg ./mnt/boot/grub/ # update grub config
./cpyfile.sh kernel.bin ./mnt/boot/ # update kernel
# files in fsfiles/
./cpyfile.sh fsfiles/* ./mnt/
