#!/bin/sh
set -e

make all
if [ ! -f disk.img ]; then
    ./gendiskimage.sh 65536
fi
./cpyfile.sh kernel.bin ./mnt/boot/
