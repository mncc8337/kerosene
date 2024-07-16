#!/bin/sh

rm -v *.o *.a *.d *.elf *.bin

if [ "$1" = "all" ]; then
    rm -v disk.img sos.iso
    sudo rm -v -r mnt
fi

echo "job done"
