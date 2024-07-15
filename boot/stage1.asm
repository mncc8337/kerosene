[org 0x7c00]

jmp first_stage

times 3 - ($ - $$) db 0x90

OEMname:           db    "mkfs.fat"
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

; disk access packet used by load_sectors routine
dap:
dap_packet_size:     db 0x10
dap_reserved:        db 0x0
dap_sectors_number:  dw 0x0
dap_buf_off:         dw 0x0
dap_buf_seg:         dw 0x0
dap_lba_low:         dd 0x0
dap_lba_high:        dd 0x0

; in:
;   dx:bx = buffer address
;   cx = start sector
;   ax = number of sectors
load_sectors:
    ;mov word [dap_lba_high], 0
    mov word [dap_lba_low], cx
    mov word [dap_buf_seg], dx
    mov word [dap_buf_off], bx
    mov word [dap_sectors_number], ax
	mov si, dap
	mov dl, [BOOT_DRIVE]
	mov ah, 0x42
	int 0x13
	jc load_sector_error
	ret
load_sector_error:
	mov si, DISK_ERROR_MSG
	call print_string
	cli
	hlt
DISK_ERROR_MSG: db "disk read error, system halted", 0

; in: push in order
;   push1 = buffer segment
;   push2 = buffer offset
;   push3 = file name
; out:
;   di = entry address
find_file:
    mov bx, sp
    lea ebp, [ss:bx]
    mov cx, word [numRootDirEntries]
    mov di, word [ROOT_DIR]
    mov si, word [ebp + 2]
.loop_find_file:
    push cx
    mov cx, 11
    push di
    push si
    rep cmpsb
    pop si
    pop di
    pop cx
    je .found_file
    add di, 32 ; go to next entry
    loop .loop_find_file
.no_file:
    ; SI now hold the file name but there is no null terminating char
    ; so we need to manually add it
    mov byte [si + 11], 0
    call print_string
    mov si, FILE_NOT_FOUND_MSG
    call print_string
    cli
    hlt
.found_file:
    mov ax, word [di + 0x1a] ; starting cluster of file
    mov di, ax ; store in DI
    ret
FILE_NOT_FOUND_MSG: db " not found. system halted", 0

boot_stage1_msg: db "stage 1 loaded", 0xd, 0xa, 0

BOOT_DRIVE: db 0
ROOT_DIR: dw 0x8200
FAT_DIR: dw 0x8500
DATA_SECTOR: dw 0

stage2f_name: db "STAGE2  BIN"
kernelf_name: db "KERNEL  ELF"

[bits 16]
first_stage:
    mov [BOOT_DRIVE], dl ; remember boot drive stored in dl

    xor ax, ax
    mov ds, ax
    mov es, ax

    mov bx, 0x7c00 ; set the stack
    cli
    mov ss, bx
    mov sp, ax
    sti
    cld

    ; clear screen
    mov ah, 0x00
    mov al, 0x03
    int 0x10

    mov si, boot_stage1_msg
    call print_string

    mov ax, word [FAT_DIR]
    mov fs, ax

    ; get root dir size
    mov ax, 32
    mul word [numRootDirEntries]
    div word [bytesPerSector]
    push ax

    ; get root dir location
    xor ax, ax
    mov al, byte [numFAT]
    mul word [numFATsectors]
    add ax, word [reservedSectors]
    mov word [DATA_SECTOR], ax
    add word [DATA_SECTOR], cx
    push ax

    ; load root dir
    xor cx, cx
    xor ax, ax
    xor dx, dx
    mov bx, [ROOT_DIR]
    pop cx
    pop ax
    call load_sectors

    ; get FAT size
    xor ax, ax
    mov al, byte [numFAT]
    mul WORD [numFATsectors]
    push ax

    ; get FAT location
    push word [reservedSectors]

    ; load FAT
    xor cx, cx
    xor ax, ax
    xor dx, dx
    mov bx, [FAT_DIR]
    pop cx
    pop ax
    call load_sectors

    ; find and load the second stage
    push word 0 ; buffer segment
    push word SECOND_STAGE_ADDR ; buffer offset
    push word stage2f_name ; file name
    call find_file

    ; TODO: load file (impossible, for me at least)

    jmp $

    ; now jump to stage 2
    jmp SECOND_STAGE_ADDR

times 510 - ($ - $$) db 0
dw 0xaa55
