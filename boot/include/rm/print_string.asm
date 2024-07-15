; in
;   si = string to print
print_string:
    pusha
    mov ah, 0xe
.print:
    lodsb
    cmp al, 0x0
    je .print_done

    int 0x10
    jmp .print
.print_done:
    popa
    ret
