#!/bin/sh

# load backup if available
if [ -f "${BIN_DIR}disk-backup.img" ]; then
    cp ${BIN_DIR}disk-backup.img ${BIN_DIR}disk.img
    echo "backup loaded"
    exit
fi

# unmount mnt
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
LOOP_DEV=$(sudo losetup --find --partscan ${BIN_DIR}disk.img --show)
if [ -z "$LOOP_DEV" ]; then
    echo "failed to set up loop device"
    exit 1
fi
echo "using loop device $LOOP_DEV"

sudo mkdosfs -F32 -f 2 ${LOOP_DEV}p1
sudo mount --onlyonce ${LOOP_DEV}p1 ./mnt

sudo grub-install --target=i386-pc \
                  --root-directory=./mnt \
                  --boot-directory=./mnt/boot \
                  --no-floppy \
                  --modules="normal part_msdos fat multiboot" \
                  ${LOOP_DEV}

sudo mkdir -p ./mnt/boot/grub
sudo cp grub.cfg ./mnt/boot/grub

sudo umount ./mnt
sudo losetup -d ${LOOP_DEV}

cp ${BIN_DIR}disk.img ${BIN_DIR}disk-backup.img

sync

echo "generated ${BIN_DIR}disk.img"
