#!/bin/sh

# load backup if available
if [ -f "${BIN_DIR}disk-backup.img" ]; then
    cp ${BIN_DIR}disk-backup.img ${BIN_DIR}disk.img
    echo "backup loaded"
    exit
fi

sudo umount ./mnt

rm ${BIN_DIR}disk.img
mkdir -p mnt

set -e

disksize_KiB=$DISK_IMAGE_SIZE

if [ -n "$1" ]; then
    disksize_KiB=$1
fi

echo making disk image
echo ------------------------------------------------------

dd if=/dev/zero of=${BIN_DIR}disk.img bs=1024 count=$disksize_KiB

fdisk ${BIN_DIR}disk.img << EOF
n
p
1


a
w
EOF

echo installing grub
echo ------------------------------------------------------

# find the first free loop device (/dev/loop0)
# and create /dev/loop0p1 for the first partition
OUTPUT=$(udisksctl loop-setup --no-user-interaction -f ${BIN_DIR}disk.img)
LOOP_DEV=$(echo "$OUTPUT" | grep -oP '/dev/loop[0-9]+')
if [ -z "$LOOP_DEV" ]; then
    echo "failed to set up loop device"
    exit 1
fi
echo "using loop device $LOOP_DEV"

sudo mkdosfs -F32 -f 2 ${LOOP_DEV}p1
MOUNT_OUT=$(udisksctl mount --no-user-interaction -b ${LOOP_DEV}p1)
MOUNT_POINT=$(echo "$MOUNT_OUT" | awk '{print $4}' | sed 's/\.$//')

sudo grub-install --target=i386-pc \
                  --root-directory="$MOUNT_POINT" \
                  --boot-directory="$MOUNT_POINT"/boot \
                  --no-floppy \
                  --modules="normal part_msdos fat multiboot" \
                  ${LOOP_DEV}

mkdir -p "$MOUNT_POINT"/boot/grub
cp grub.cfg "$MOUNT_POINT"/boot/grub

udisksctl unmount --no-user-interaction -b ${LOOP_DEV}p1
udisksctl loop-delete --no-user-interaction -b ${LOOP_DEV}

cp ${BIN_DIR}disk.img ${BIN_DIR}disk-backup.img

sync

echo "generated ${BIN_DIR}disk.img"
