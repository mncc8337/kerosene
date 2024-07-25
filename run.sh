#!/bin/sh

QEMUFLAGS="-m 128M \
          -debugcon stdio \
          -no-reboot \
          -accel tcg,thread=single \
          -usb"

if [ "$1" == "debug" ]; then
    qemu-system-i386 -hda disk.img $QEMUFLAGS -s -S &
    gdb bin/kernel.bin \
        -ex 'target remote localhost:1234' \
        -ex 'layout src' \
        -ex 'layout reg' \
        -ex 'skip tty_scroll_screen' \
        -ex 'skip memcpy' \
        -ex 'skip memset' \
        -ex 'skip strcmp' \
        -ex 'skip printf' \
        -ex 'skip puts' \
        -ex 'skip putchar' \
        -ex 'skip ata_pio_LBA28_access' \
        -ex 'break kmain' \
        -ex 'continue'
else
    qemu-system-i386 -hda disk.img $QEMUFLAGS
fi
