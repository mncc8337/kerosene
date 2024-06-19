org 0x7c00
bits 16

jmp main

%include "print.asm"

msg1: db "Hello world!", ENDL, ENDS
msg2: db "it ran yay!", ENDS

main:
    call cls

    mov bx, msg1
    call print

    mov bx, msg2
    call print

    hlt

times 510-($-$$) db 0
dw 0xaa55
