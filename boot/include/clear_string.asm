; clear first cx bytes of si
clear_string:
    cmp cx, 0x0
    je .end_clear_string

    mov al, 0x0
    mov [si], al
    inc si
    dec cx
    jmp clear_string
.end_clear_string:
    ret
