kernel: jmp k_main

%include "16-bits/print.asm"
%include "16-bits/string.asm"
kernel_msg: db "Welcome to the S OS!", ENDL, 0
prompt: db "[kernel@sos]$ ", 0

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

times 512 - ($ - $$) db 0
