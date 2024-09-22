#!/bin/sh

# load backup if available
if [ -f "${BIN_DIR}disk-backup.img" ]; then
    cp ${BIN_DIR}disk-backup.img ${BIN_DIR}disk.img
    echo "backup loaded"
    exit
fi

# unmount all the things
sudo umount ./mnt
sudo losetup -d /dev/loop0
sudo losetup -d /dev/loop1

rm ${BIN_DIR}disk.img
mkdir -p mnt

set -e

disksize_KiB=65536

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

sudo losetup /dev/loop0 ${BIN_DIR}disk.img
sudo losetup /dev/loop1 ${BIN_DIR}disk.img -o 1048576 # 1024^2
sudo mkdosfs -F32 -f 2 /dev/loop1
sudo mount --onlyonce /dev/loop1 ./mnt

sudo grub-install --root-directory=./mnt --boot-directory=./mnt/boot --no-floppy --modules="normal part_msdos fat multiboot" /dev/loop0

sudo mkdir -p ./mnt/boot/grub
sudo cp grub.cfg ./mnt/boot/grub

sudo umount ./mnt
sudo losetup -d /dev/loop0
sudo losetup -d /dev/loop1

cp ${BIN_DIR}disk.img ${BIN_DIR}disk-backup.img

sync

echo "generated ${BIN_DIR}disk.img"
