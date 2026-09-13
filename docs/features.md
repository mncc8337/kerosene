# Features
## Kernel
- [x] handle exception interrupts
- [x] handle interrupts send by PIC
- [x] higher half kernel
- [x] built-in kernel stack trace
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
    + [x] virtual memory manager
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
    + [x] ramfs (fs that lives in ram rather than disk drive)
- vfs
    + [x] node tree
    + [x] find node in tree
    + [x] unused node clean up
    + [x] node open/close
    + [x] node read/write
    + [x] iterate directories
    + [x] node seek
    + [x] bind mount
    + [ ] symlink
    + [ ] permission
### Userland
- [x] TSS setup
- [x] enter usermode
- syscall
    + [x] putchar() and variants
    + [x] current time
    + scheduler:
        + [x] kill()
        + [x] sleep()
        + [x] yield()
        + [x] spawn()
        + [x] attach()
    + vfs
        + [x] open/close
        + [x] read/write
        + [x] seek
        + [x] lock/unlock
        + [ ] remove file
        + [ ] mkdir
        + [ ] rmdir
        + [ ] dir iteration
        + [x] bind mount
    + semaphore
        + [x] create
        + [x] acquire
        + [x] release
    + memory: first fit allocator
        + [ ] malloc
        + [ ] free
- scheduler
    + [x] load/create process
    + [x] load and save process state
    + [x] basic process scheduling (round robin)
    + [x] process terminating
    + [x] spinlock
    + [x] semaphore
    + [x] spawn a process
    + [x] get spawned process exit code
    + [x] pass argv to spawned process
    + [x] pass envp to spawned process
## shell
currently its a minimal shell that can run program based on absolute path
## coreutils
- [x] echo
