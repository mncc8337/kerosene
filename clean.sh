#!/bin/sh

rm *.o *.a *.d *.elf *.bin

if [ "$1" = "all" ]; then
    rm disk.img sos.iso
    sudo rm -r mnt
fi

echo "job done"
