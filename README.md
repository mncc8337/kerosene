# Kerosene
A WIP hobby x86 monolithic OS.
## Features
- PS/2 keyboard driver
- basic framebuffer video driver
- PIT
- ATA PIO mode
- FAT32 filesystem
- ramfs filesystem
## Build and run
### Prerequisite
- A [GCC cross compiler](https://wiki.osdev.org/GCC_Cross-Compiler). although preinstalled GCC on linux will compile it just fine, the osdev wiki said we should use a cross compiler to avoid any unexpected errors.
    + if you are lazy to install one, use `make all NO_CROSS_COMPILER=1` to compile using linux GCC.
    + if you have cross compiling tools then please check and correct the `CROSS_COMPILER_LOC` in `.env`
- nasm
- dosfstools
- grub (and xorriso to gen iso image)
- qemu
### Build and run
> [!Note]
> - This project is only built and tested on a linux machine (arch btw). Maybe on Windows with WSL or other OS will build and run just fine, though it is not guaranteed.
> - some scripts in `script/` will need sudo privilege to setup loopback device for the hard disk image.

make sure to source enviroment vars before doing anything. makefile and shell scripts depend heavily on them.
```
cp example.env .env
source .env
```
#### build
```
chmod +x script/*.sh
make all
```
#### run
```
make run
```
#### make iso
```
./script/geniso.sh
```
#### edit the fs files
- first mount the disk device
```
./script/mount-device.sh
cd mnt/
```
- then do some stuffs in `mnt/`
- finally unmount. you also want to run this if you ever encounter errors like disk image is in use or whatever
```
./script/umount-device.sh
```
- if you want to copy some files/dirs and dont want to run mount-device and then umount-device
```
./script/cpyfile.sh file/or/directory/in/somewhere ./mnt/some/DIRECTORY/in/the/disk
```
Files and directories in `fsfiles/` will be automatically copied into the disk image after running `make all`.
### Running on real hardware
You can either make an iso `./script/geniso.sh` and burn it to an usb or use `sudo dd if=disk.img of=/dev/sdX && sync` to burn the disk to an usb to run the OS.
> [!Caution]
> - I am not responsible for any damage caused to your machine by the OS. Try this with your own risk!
> - I do not test the runability of the OS on every commits so don't expect it to run normaly. Also i do not own many pc to test properly so it maybe only works on my pc.
## Progress
### Kernel stuffs
- [x] load GDT in the kernel
- [x] load IDT in the kernel
- [x] handle exception interrupts
- [x] handle interrupts send by PIC
- [x] higher half kernel
- [ ] multiprocessor support
### Hardware drivers
- PS/2
    + PS/2 keyboard driver
        + [x] get key scancode
        + [x] translate scancode to keycode
        + [x] LED indicating
    + [ ] PS/2 mouse driver
- memory manager
    + physical memory manager
        + [x] bitmap allocator
        + [ ] better allocator
    + [x] virtual memory manager
    + heap
        + [x] first-fit allocator
        + [ ] better allocator
- ATA
    + [x] PIO mode
    + [ ] SATA
- [x] CMOS and RTC: get datetime
- [ ] APCI
- counter
    + PIT
        + [x] generate ticks
        + [x] PC speaker beep beep boop boop
    + [ ] APIC
    + [ ] HPET
- video
    + [x] vga
    + framebuffer
        + [x] plot pixel
        + [x] render psf fonts
- USB
    + [ ] keyboard
    + [ ] mouse
- [ ] sound
- [ ] networking
### Filesystem
- [x] MBR support
- [ ] GPT support
- fs
    + [x] FAT32
    + [ ] ext2
    + [x] ramfs
- vfs
    + [x] node tree
    + [x] find node in tree
    + [x] unused node clean up
    + [ ] symlink
    + [ ] permission
### Userland
- [x] TSS setup
- [x] enter usermode
- syscall
    + [x] putchar()
    + [x] current time
    + [x] terminate process
    + [x] sleep()
    + [x] open file
    + [ ] read file
    + [ ] write file
    + [x] close file
- process manager
    + [x] load process
    + [x] load and save process state
    + [x] basic process scheduling
    + [x] process terminate
    + [x] spinlock
    + [ ] semaphore
- [x] load and run ELF files
## Known bugs
- ATA PIO mode initialization occasionally failed: address mark not found
- `video_framebuffer_print_char` sometime cause a pagefault
- `print_debug` gives gliberrish result if put in many formats
## Learning resources
> [!Tip]
> Anything related to osdev can be found on [the osdev wiki](http://wiki.osdev.org/Expanded_Main_Page)
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
