#!/bin/sh
set -e

# get the absolute path to the disk image
IMG_PATH=$(realpath "${BIN_DIR}disk.img")
echo $IMG_PATH
echo $BIN_DIR

# find the loop device associated with that file
LOOP_DEV=$(sudo losetup --list --noheadings -O NAME,BACK-FILE | grep "$IMG_PATH" | awk '{print $1}')

if [ -z "$LOOP_DEV" ]; then
    echo "no loop device found for $IMG_PATH"
    echo "did you run the create_disk script first?"
    exit 1
fi

echo "found device: $LOOP_DEV"
echo "cleaning up ..."
sudo umount ./mnt
sudo losetup -d $LOOP_DEV
