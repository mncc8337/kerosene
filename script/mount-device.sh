#!/bin/sh
set -e

echo "setting up loop device..."
OUTPUT=$(udisksctl loop-setup --no-user-interaction -f ${BIN_DIR}disk.img)
LOOP_DEV=$(echo "$OUTPUT" | grep -oP '/dev/loop[0-9]+')
if [ -z "$LOOP_DEV" ]; then
    echo "failed to set up loop device"
    exit 1
fi

echo "using loop device $LOOP_DEV"

MOUNT_OUT=$(udisksctl mount --no-user-interaction -b ${LOOP_DEV}p1)
MOUNT_POINT=$(echo "$MOUNT_OUT" | awk '{print $4}' | sed 's/\.$//')

if [ -z "$MOUNT_POINT" ]; then
    echo "failed to mount loop device"
    udisksctl loop-delete --no-user-interaction -b ${LOOP_DEV}
    exit 1
fi

rm -rf ./mnt
ln -s "$MOUNT_POINT" ./mnt

echo "mounted"