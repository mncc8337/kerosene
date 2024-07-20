# S OS
a x86 os written from scratch  
this is an attempt of me to learn about osdev  
it's a working-in-progress project and should not be used as references for osdev (it's full of bad practices if you ask)  
## features
- ~~a (bad/suck/horrible) custom 2-stage bootloader~~ i switched to GRUB
- a proper keyboard driver (which lacks of LED indicating, shortcuts support lol)
- it can print text! (with colors!)
- yeah that's all
## build and run
to compile and run the project you will need:
- a [GCC cross compiler](https://wiki.osdev.org/GCC_Cross-Compiler). although a normal x86_64 elf GCC will compile it without any errors (by adding some flags, see the commented CFLAGS lines in Makefile), the osdev wiki said we should use a cross compiler to avoid any unexpected errors
- nasm
- grub (and xorriso to gen iso image)
- qemu  
to build `./build.sh`. to run `./run.sh`. to add file to the disk image `./cpyfile.sh $filename ./mnt/path/to/dir`  
these scripts will need sudo privilege to setup loopback device for the hard disk image.
> [!WARNING]  
> YOU SHOULD NOT RUN THE OS ON REAL HARDWARE.  
> if you wish to do it any way, run `sudo dd if=disk.img of=/dev/sdX && sync` (MAKE SURE /dev/sdX IS AN USB DEVICE) or any program to burn the disk image into an usb device, restart the pc and choose the usb in the boot menu.
## progress
- [x] boot to the kernel
- [x] print some text in the kernel
- [x] 2-stage bootloader
- [x] load GDT in the kernel
- [x] load IDT in the kernel
- [x] handle exception interrupt
- [x] handle interrupts send by PIC
- [ ] keyboard driver
    + [x] get key scancode
    + [x] translate scancode to keycode
    + [ ] handle key combination
- [x] play with the PIT
- [ ] memory manager
    + [x] physical memory manager
    + [x] virtual memory manager
    + [ ] the heap
- [ ] get current datetime (RTC, CMOS)
- [x] support multiboot (i switched to GRUB so it should be lol)
- [x] read disk
- [x] MBR support
- [ ] GPT support
- [ ] filesystem
    - [ ] FAT32
        - [x] detect
        - [x] list
        - [ ] make dir
        - [x] read
        - [ ] write
    - [ ] ext2
        - [ ] detect
        - [ ] list
        - [ ] make dir
        - [ ] read
        - [ ] write
- [ ] load and run ELF file
- [x] load kernel with ELF binary instead of flat binary
- [ ] higher half kernel
- [ ] multi-processing
- [ ] userland
- [ ] APCI
- [ ] mouse driver
- [ ] GUI
    - [ ] render rectangle
    - [ ] render mouse
    - [ ] render image
    - [ ] render fonts
- [ ] sound
- [ ] video
- [ ] im not gonna touch networking
- [ ] port some program
    + [ ] GNU GCC
## known bugs
- ATA PIO mode initialization some time failed (very rare)
## learning resources
- https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf
- http://www.osdever.net/bkerndev/Docs/gettingstarted.htm
- http://wiki.osdev.org/Expanded_Main_Page
- http://www.brokenthorn.com/Resources/OSDevIndex.html
- https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
- https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/fatgen103.doc
