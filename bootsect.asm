[org 0x7c00]

boot: jmp b_main

%include "print.asm"
%include "disk.asm"
drive_msg: db "booted into drive ", 0
kernel_msg: db "kernel loaded", ENDL, 0

BOOT_DRIVE: db 0
KERNEL_OFFSET: dw 0x1000

b_main:
    mov [BOOT_DRIVE], dl ; remember boot drive stored in dl

    mov bp, 0x9000 ; setup the stack.
    mov sp, bp

    mov si, drive_msg
    call print_string

    mov ax, 0
    mov dx, [BOOT_DRIVE]
    call print_hex

    call print_new_line

    [bits 16]

    ; load sectors to es:bx
    mov ax, KERNEL_OFFSET
    mov es, ax
    mov bx, 0x0
    ; sector number to load
    mov dh, 15
    mov dl, [BOOT_DRIVE]
    call disk_load

    mov si, kernel_msg
    call print_string

    jmp KERNEL_OFFSET:0x0

times 510 - ($ - $$) db 0
dw 0xaa55
