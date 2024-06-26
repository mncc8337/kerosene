# S OS
another simple/suck/shitty/stupid project  
## build and run
you will need `nasm` to build and `qemu` to run the OS  
to build just run `make`  
to test it just run `make run`
> [!WARNING]  
> DO NOT RUN THE OS ON REAL HARDWARE.  
> if you wish to do it any way, run `sudo dd if=disk.img of=/dev/sdX && sync` (MAKE SURE /dev/sdX IS AN USB DEVICE) or any program to burn the disk image into an usb device, restart the pc and choose the usb in the boot menu.
## learning resources
- https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf
- http://www.osdever.net/bkerndev/Docs/gettingstarted.htm
- https://github.com/garciart/hello-os
- http://wiki.osdev.org/Expanded_Main_Page
