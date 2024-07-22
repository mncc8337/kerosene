#!/bin/sh
set -e

make all
if [ ! -f disk.img ]; then
    ./gendiskimage.sh 65536
fi
./cpyfile.sh grub.cfg ./mnt/boot/grub/ # update grub config
./cpyfile.sh bin/kernel.bin ./mnt/boot/ # update kernel
for file in fsfiles/*; do
    ./cpyfile.sh $file ./mnt/
done
