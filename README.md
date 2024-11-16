# Kerosene
A hobby x86 monolithic OS written in C  
This is an attempt of me to learn about osdev  
## Features
- PS/2 keyboard driver
- ATA PIO mode
- FAT32 filesystem support
- basic VESA driver (i think)
## Build and run
### Prerequisite
- A [GCC cross compiler](https://wiki.osdev.org/GCC_Cross-Compiler). although a preinstalled GCC on linux will compile it just fine, the osdev wiki said we should use a cross compiler to avoid any unexpected errors.
    + if you are lazy to install one, use `make all NO_CROSS_COMPILER=1` to compile using linux GCC (please do not report any errors if you use this option)
    + if you use a cross compiler then please check and correct the `CROSS_COMPILER_LOC` in `.env`
- nasm
- dosfstools
- grub (and xorriso to gen iso image)
- qemu
### Build and run
```
# build
chmod +x script/*.sh
source .env
make all
```
```
# run
./script/run.sh # or make run
```
```
# make iso
./script/geniso.sh
```
```
# editing the disk files
./script/mount-device.sh
cd mnt/
## do some stuffs here
./script/umount-device.sh # you also want to run this if you ever encountered errors like disk image is in use or whatever
```
```
# if you want to copy some files/dirs and dont want to run mount-device and then umount-device
./script/cpyfile.sh file/or/directory/in/somewhere ./mnt/some/DIRECTORY/in/the/disk
```
Files and directories in `fsfiles/` will be automatically copied into the disk image after running `make all`.  
Note that scripts in `script/` will need sudo privilege to setup loopback device for the hard disk image.
### Running on real hardware
You can either make an iso `./script/geniso.sh` and burn it to an usb or use `sudo dd if=disk.img of=/dev/sdX && sync` to burn the disk to an usb to run the OS. Note that ISO 9660 FS and usb driver are not implemented so the OS will perform filesystem commands on whatever partitions with FAT32 fs it found so just dont perform fs command and you will be fine.
> [!NOTE]
> I am not responsible for any damage caused to your machine by the OS. Try this with your own risk!
## Progress
### Baby first step
- [x] basic bootloader
- [x] load the kernel
- [x] 2-stage bootloader
- [x] print some text in the kernel
- [x] switch to GRUB
### Kernel stuffs
- [x] load GDT in the kernel
- [x] load IDT in the kernel
- [x] handle exception interrupt
- [x] handle interrupts send by PIC
- [x] load kernel with ELF binary instead of flat binary
- [x] support multiboot
- [x] higher half kernel
- [ ] multiprocessor support
### Hardware drivers
- [ ] PS/2 keyboard driver
    + [x] get key scancode
    + [x] translate scancode to keycode
    + [x] LED indicating
- [x] PIT
    - [x] generate ticks
    - [x] PC speaker (beep beep boop boop)
- [x] memory manager
    - [x] physical memory manager: bitmap allocator
    - [x] virtual memory manager
    - [x] the heap: first-fit allocator
- [ ] ATA
    - [x] PIO mode
- [x] CMOS and RTC: get datetime
- [ ] APCI
- [ ] APIC
- [ ] HPET
- [ ] PS/2 mouse driver
- [x] VESA
    - [x] plot pixel
    - [x] render psf fonts
- [ ] USB
- [ ] sound
- [ ] im not gonna touch networking
### Filesystem
- [x] MBR support
- [ ] GPT support
- [x] FAT32 fs
    - [x] detect
    - [x] list
    - [x] make dir
    - [x] remove dir
    - [x] touch files
    - [x] read
    - [x] write file
    - [x] move
    - [x] copy
- [ ] ext2 fs
- [ ] vfs
### Userland
- [x] TSS setup
- [x] enter usermode
- [ ] syscall
    - [x] putchar
    - [x] current time
    - [x] terminate process
    - [x] sleep()
    - [ ] read file
    - [ ] write file
- [ ] process manager
    - [x] load process
    - [x] load and save process state
    - [x] basic process scheduling
    - [x] process terminate
    - [x] spinlock
    - [ ] semaphore
- [x] load and run ELF file
## Known bugs
- ATA PIO mode initialization some time failed (very rare): address mark not found
- `print_debug` gives gliberrish result if put in many formats
- global variable would cause conflict between processes (see file_op.c fat32.c strtok.c)
## Learning resources
Note that anything related to osdev are on [the osdev wiki](http://wiki.osdev.org/Expanded_Main_Page)
### Great tutorials
- https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf
- http://www.osdever.net/bkerndev/Docs/gettingstarted.htm
- http://www.brokenthorn.com/Resources/OSDevIndex.html
- https://web.archive.org/web/20160313134414/http://www.jamesmolloy.co.uk/tutorial_html/index.html
### Documents
- https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
- https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
- https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/fatgen103.doc
- http://www.skyfree.org/linux/references/ELF_Format.pdf
