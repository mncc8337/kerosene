[org 0x7c00]

boot: jmp b_main
times 3 - ($ - $$) db 0x90   ; support 2 or 3 byte encoded JMPs before BPB.
; fake BPB filed with 0xaa
times 59 db 0xaa

%include "print.asm"

drive_msg: db "booted into drive ", 0
drive_type_floppy_msg: db " (floppy)", ENDL, 0
drive_type_hd_msg: db " (hard disk)", ENDL, 0

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
    mov [BOOT_DRIVE], dl ; remember boot drive stored in dl

    mov bp, 0x9000 ; setup the stack.
    mov sp, bp

    mov si, drive_msg
    call print_string

    mov dx, [BOOT_DRIVE]
    call print_hex

    ; detect drive type
    ; if BOOT_DRIVE is less than 0x80 then it is floppy
    ; else it is hard disk
    mov dx, [BOOT_DRIVE]
    cmp dx, 0x80
    jl type_floppy
    jmp type_hd

    type_floppy:
    mov si, drive_type_floppy_msg
    call print_string
    jmp done_check_type

    type_hd:
    mov si, drive_type_hd_msg
    call print_string

    done_check_type:
    [bits 16]

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

times 510 - ($ - $$) db 0
dw 0xaa55
