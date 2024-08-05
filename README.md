# S OS
a x86 os written from scratch  
this is an attempt of me to learn about osdev  
it's a working-in-progress project and should not be used as references for osdev (it's full of bad practices if you ask)  
## features
- a proper keyboard driver (which lacks of LED indicating, shortcuts support lol)
- it can print text! (with colors!)
- FAT32 filesystem support
- yeah that's all
## build and run
### prerequisite
- a [GCC cross compiler](https://wiki.osdev.org/GCC_Cross-Compiler). although a normal x86_64 elf GCC will compile it without any errors (by adding some flags, see the commented CFLAGS lines in Makefile), the osdev wiki said we should use a cross compiler to avoid any unexpected errors
- nasm
- grub (and xorriso to gen iso image)
- qemu
### build and run
```
# build
chmod +x script/*.sh
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
note that scripts in `script/` will need sudo privilege to setup loopback device for the hard disk image.
> [!WARNING]  
> YOU SHOULD NOT RUN THE OS ON REAL HARDWARE.  
> if you wish to do it any way, run `sudo dd if=disk.img of=/dev/sdX && sync` (MAKE SURE /dev/sdX IS AN USB DEVICE) or any program to burn the disk image into an usb device, restart the pc and choose the usb in the boot menu.
## progress
### baby first step
- [x] basic bootloader
- [x] load the kernel
- [x] 2-stage bootloader
- [x] print some text in the kernel
- [x] switch to GRUB
### kernel stuffs
- [x] load GDT in the kernel
- [x] load IDT in the kernel
- [x] handle exception interrupt
- [x] handle interrupts send by PIC
- [x] load kernel with ELF binary instead of flat binary
- [x] support multiboot
- [ ] higher half kernel
### hardware drivers
- [ ] keyboard driver
    + [x] get key scancode
    + [x] translate scancode to keycode
    + [ ] LED indicating
- [ ] PIT
    - [x] generate ticks
    - [ ] PC speaker (beep beep boop boop)
- [ ] memory manager
    - [x] physical memory manager
    - [x] virtual memory manager
    - [ ] the heap
- [ ] ATA
    - [x] PIO mode
    - [ ] to be updated
- [ ] CMOS
    - [ ] get datetime
- [ ] APCI
- [ ] mouse driver
- [ ] GUI
    - [ ] render rectangle
    - [ ] render fonts
    - [ ] render image
    - [ ] render mouse
- [ ] sound
- [ ] video
- [ ] im not gonna touch networking
### filesystem
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
### userland
- [ ] load and run ELF file
- [ ] multi-processing
- [ ] userland
    - [x] TSS setup
    - [ ] to be updated
- [ ] port some program
    - [ ] GNU GCC
    - [ ] to be updated
### misc
- [ ] get current timestamp
- [ ] set file timestamp
## known bugs
- ATA PIO mode initialization some time failed (very rare): address mark not found
- sometime keyboard is not working (seem like it only appears in QEMU)
- enter_usermode() crash after run `iret`
## learning resources
note that anything related to osdev are on [the osdev wiki](http://wiki.osdev.org/Expanded_Main_Page)
### great tutorials
- https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf
- http://www.osdever.net/bkerndev/Docs/gettingstarted.htm
- http://www.brokenthorn.com/Resources/OSDevIndex.html
- https://web.archive.org/web/20160313134414/http://www.jamesmolloy.co.uk/tutorial_html/index.html
### documents
- https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
- https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/fatgen103.doc
