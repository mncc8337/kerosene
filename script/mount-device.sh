#!/bin/sh
set -e

sudo losetup /dev/loop1 ${BIN_DIR}disk.img -o 1048576 # 1024^2
sudo mount --onlyonce /dev/loop1 ./mnt
