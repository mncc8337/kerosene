#!/bin/sh
set -e

IMG_PATH=$(realpath "${BIN_DIR}disk.img")

LOOP_DEV=$(losetup --list --noheadings -O NAME,BACK-FILE | grep "$IMG_PATH" | awk '{print $1}' | head -n 1)

if [ -z "$LOOP_DEV" ]; then
    echo "no loop device found for $IMG_PATH"
    echo "did you run the create_disk script first?"
    exit 1
fi

echo "found device: $LOOP_DEV"
echo "cleaning up ..."

udisksctl unmount --no-user-interaction -b ${LOOP_DEV}p1 || true
udisksctl loop-delete --no-user-interaction -b ${LOOP_DEV} || true
rm -f ./mnt
