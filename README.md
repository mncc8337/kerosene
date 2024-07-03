# S OS
another simple/suck/shitty/stupid project  
## build and run
to compile and run the project you will need:
- a [GCC cross compiler](https://wiki.osdev.org/GCC_Cross-Compiler). although a normal x86_64 elf GCC will compile it without any errors (by adding some flags, see the commented CFLAGS lines in Makefile), the osdev wiki said we should use a cross compiler to avoid any unexpected errors
- nasm
- qemu  
to build and test it just run `make run`. make sure to correct the location of your cross compiler `CROSS_COMPILER_LOC` in the first few lines in `Makefile`  
> [!WARNING]  
> DO NOT RUN THE OS ON REAL HARDWARE.  
> if you wish to do it any way, run `sudo dd if=disk.img of=/dev/sdX && sync` (MAKE SURE /dev/sdX IS AN USB DEVICE) or any program to burn the disk image into an usb device, restart the pc and choose the usb in the boot menu.
## learning resources
- https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf
- http://www.osdever.net/bkerndev/Docs/gettingstarted.htm
- https://github.com/garciart/hello-os
- http://wiki.osdev.org/Expanded_Main_Page
- http://www.brokenthorn.com/Resources/OSDev17.html
