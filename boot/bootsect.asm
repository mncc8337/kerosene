[org 0x7c00]

boot: jmp b_main

times 3 - ($ - $$) db 0x90   ; Support 2 or 3 byte encoded JMPs before BPB.

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


%include "print_string.asm"
%include "print_hex.asm"
%include "disk_load.asm"
%include "switch_to_pm.asm"
%include "print_string_pm.asm"

[bits 16]

new_line: db 0xd, 0xa, 0

boot_msg: db "booted into drive ", 0
kernel_loaded_msg: db "kernel loaded", 0xd, 0xa, 0

BOOT_DRIVE: db 0

b_main:
    cli
    cld

    xor ax, ax
    mov ds, ax
    mov es, ax

    mov ss, ax

    mov bp, 0x7c00 ; set the stack
    mov sp, bp

    mov [BOOT_DRIVE], dl ; remember boot drive stored in dl

    mov si, boot_msg
    call print_string
    mov dx, [BOOT_DRIVE]
    xor dh, dh
    call print_hex
    mov si, new_line
    call print_string

    ; load kernel
    mov bx, KERNEL_OFFSET
    mov dh, KERNEL_PADDING
    mov dl, [BOOT_DRIVE]
    call disk_load

    mov si, kernel_loaded_msg
    call print_string

    call switch_to_pm

    jmp $

[bits 32]
pm_msg: db "switched to protected mode", 0
BEGIN_PM:
    mov esi, pm_msg
    call print_string_pm

    ; now jump to the kernel and never come back
    call KERNEL_OFFSET

    jmp $

times 510 - ($ - $$) db 0
dw 0xaa55
