[org 0x7c00]
[bits 16]

boot: jmp b_main

times 3-($-$$) db 0x90   ; Support 2 or 3 byte encoded JMPs before BPB.

; Dos 4.0 EBPB 1.44MB floppy
OEMname:           db    "mkfs.fat"  ; mkfs.fat is what OEMname mkdosfs uses
bytesPerSector:    dw    512
sectPerCluster:    db    1
reservedSectors:   dw    1
numFAT:            db    2
numRootDirEntries: dw    224
numSectors:        dw    2880
mediaType:         db    0xf0
numFATsectors:     dw    9
sectorsPerTrack:   dw    18
numHeads:          dw    2
numHiddenSectors:  dd    0
numSectorsHuge:    dd    0
driveNum:          db    0
reserved:          db    0
signature:         db    0x29
volumeID:          dd    0x2d7e5a1a
volumeLabel:       db    "S_OS       "
fileSysType:       db    "FAT12   "

%include "16-bits/print.asm"

drive_msg: db "booted into drive ", 0

kernel_loaded_msg: db "kernel loaded", ENDL, 0

BOOT_DRIVE: db 0
KERNEL_OFFSET: dw 0x1000

DISK_ERROR_MSG: db "disk read error, system hang", 0
disk_load:
    push dx          ; store dx on stack so later we can recall how many sectors were request to be read
    mov ah, 0x2      ; BIOS read sector function
    mov al, dh       ; read dh sectors
    mov ch, 0x0      ; select cylinder 0
    mov dh, 0x0      ; select head 0
    mov cl, 0x2      ; start reading from second sector (after the boot sector)
    int 0x13         ; BIOS interrupt
    jc .disk_error    ; jump if error (carry flag set)

    pop dx           ; restore dx from the stack
    cmp dh, al       ; if al (sectors read) != dh (sectors expected)
    jne .disk_error   ; display error message
    ret
.disk_error:
    mov si, DISK_ERROR_MSG
    call print_string
    jmp $

b_main:
    cli
    cld

    xor ax, ax
    mov ds, ax
    mov es, ax

    mov ss, ax
    mov sp, 0x7c00

    mov [BOOT_DRIVE], dl ; remember boot drive stored in dl

    mov si, drive_msg
    call print_string

    mov dx, [BOOT_DRIVE]
    call print_hex

    call print_new_line

    ; load kernel to KERNEL_OFFSET:0x0

    mov ax, KERNEL_OFFSET
    mov es, ax
    mov bx, 0x0
    ; sector number to load
    mov dh, 1 ; number of sector must be smaller or equal to kernel.bin if booted on a HDD
    mov dl, [BOOT_DRIVE]
    call disk_load

    mov si, kernel_loaded_msg
    call print_string

    jmp KERNEL_OFFSET:0x0

    jmp $

times 510 - ($ - $$) db 0
dw 0xaa55
