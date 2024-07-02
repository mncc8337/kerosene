[org 0x7c00]

jmp first_stage

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

%include "rm/print_string.asm"
%include "rm/disk_load.asm"

boot_stage1_msg: db "first stage loaded", 0xd, 0xa, 0
kernel_loaded_msg: db "kernel loaded", 0xd, 0xa, 0

BOOT_DRIVE: db 0

[bits 16]
first_stage:

    xor ax, ax
    mov ds, ax
    mov es, ax

    mov bx, 0x7c00 ; set the stack
    cli
    mov ss, bx
    mov sp, ax
    sti
    cld

    mov [BOOT_DRIVE], dl ; remember boot drive stored in dl

    ; clear screen
    mov ah, 0x00
    mov al, 0x03
    int 0x10

    mov si, boot_stage1_msg
    call print_string

    ; load second stage
    mov bx, SECOND_STAGE_ADDR
    mov cl, 0x2
    mov dh, 2
    mov dl, [BOOT_DRIVE]
    call disk_load

    ; load kernel
    mov bx, KERNEL_ADDR
    mov cl, 4 ; kernel start in sector 4
    mov dh, KERNEL_SECTOR_COUNT
    mov dl, [BOOT_DRIVE]
    call disk_load

    mov si, kernel_loaded_msg
    call print_string

    ; now jump to stage 2
    jmp SECOND_STAGE_ADDR

times 510 - ($ - $$) db 0
dw 0xaa55
