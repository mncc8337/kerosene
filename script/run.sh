#!/bin/sh

QEMUFLAGS="-m 128M \
          -serial stdio \
          -no-shutdown \
          -no-reboot \
          -audiodev pa,id=speaker \
          -machine pcspk-audiodev=speaker \
          -accel tcg,thread=single \
          -usb"
          # -d int \

if [ "$1" == "debug" ]; then
    qemu-system-i386 -hda disk.img $QEMUFLAGS -s -S &
    gdb bin/kerosene.bin \
        -ex 'target remote localhost:1234' \
        -ex 'layout src' \
        -ex 'layout reg' \
        -ex 'break kmain' \
        -ex 'continue'
else
    qemu-system-i386 -drive format=raw,file=disk.img $QEMUFLAGS
fi
