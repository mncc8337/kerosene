kernel: jmp k_main

%include "print.asm"
SPACE: db " ", 0
kernel_msg: db "Welcome to the S OS!", ENDL, 0

k_main:
    push cs
    pop ds

    mov si, kernel_msg
    call print_string

.k_mainloop:
    ; read key input and print
    xor ax, ax
    int 0x16

    mov dx, ax
    call print_hex

    mov si, SPACE
    call print_string

    mov al, dl
    int 0x10

    call print_new_line

    jmp .k_mainloop
