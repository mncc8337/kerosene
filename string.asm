; set string whose memory address is stored in si to user string input
; return char count cx
get_line:
    mov cx, 0
.get_char:
    xor ax, ax
    int 0x16 ; get usr input (blocking) interrupt

    cmp al, 0x20
    jge .get_char_pass2
    jmp .not_printable
.get_char_pass2:
    cmp al, 0x7e
    jle .get_char_printable
    jmp .not_printable

.not_printable:
    ; process special key

    cmp al, 0xd ; return keycode
    je .end_get_line

    cmp al, 0x8 ; backspace keycode
    je .backspace_process

    ; else try again
    jmp .get_char

.backspace_process:
    cmp cx, 0
    je .get_char
    dec cx

    ; print BS, then space, then BS
    ; to 'delete' a previously printed char
    mov ah, 0xe
    mov al, 0x8
    int 0x10
    mov al, 0x20
    int 0x10
    mov al, 0x8
    int 0x10

    jmp .get_char

.get_char_printable:
    mov ah, 0xe ; print the typed char
    int 0x10

    mov [si], al
    inc si
    inc cx

    jmp .get_char
.end_get_line:
    ret

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
