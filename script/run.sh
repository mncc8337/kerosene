#!/bin/sh

QEMUFLAGS="-m 128M \
          -serial stdio \
          -no-shutdown \
          -no-reboot \
          -accel tcg,thread=single \
          -usb"

if [ "$1" == "debug" ]; then
    qemu-system-i386 -hda disk.img $QEMUFLAGS -s -S &
    gdb bin/kernel.bin \
        -ex 'target remote localhost:1234' \
        -ex 'layout src' \
        -ex 'layout reg' \
        -ex 'break kmain' \
        -ex 'continue'
else
    qemu-system-i386 -drive format=raw,file=disk.img $QEMUFLAGS
fi
