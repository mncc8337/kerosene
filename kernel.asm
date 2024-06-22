kernel: jmp k_main

%include "print.asm"
kernel_msg: db "Welcome to the S OS!", ENDL, 0
prompt: db "[kernel@sos]$ ", 0

%include "string.asm"
; store user line input here
USR_INPUT: times 128 db 0, 0

k_main:    
    push cs
    pop ds

    mov si, kernel_msg
    call print_string

.k_mainloop:
    mov si, prompt
    call print_string

    mov si, USR_INPUT
    call get_line
    call print_new_line

    ; process usr input here

    mov si, USR_INPUT
    call clear_string

    jmp .k_mainloop

; fill up freespace to reach 512 bytes (1 sector)
times 512 - ($ - $$) db 0
