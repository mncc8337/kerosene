#!/bin/sh

QEMUFLAGS="-m 128M \
          -serial stdio \
          -no-shutdown \
          -no-reboot \
          -audiodev pa,id=speaker \
          -machine pcspk-audiodev=speaker \
          -accel tcg,thread=single \
          -d int \
          -usb"

if [ "$1" == "debug" ]; then
    qemu-system-i386 -drive format=raw,file=${BIN_DIR}disk.img $QEMUFLAGS -s -S &
    gdb bin/kerosene.elf \
        -ex 'target remote localhost:1234' \
        -ex 'layout src' \
        -ex 'layout reg' \
        -ex 'break kmain' \
        -ex 'continue'
else
    qemu-system-i386 -drive format=raw,file=${BIN_DIR}disk.img $QEMUFLAGS
fi
