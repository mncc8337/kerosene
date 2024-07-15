[bits 32]

; in
;   edx = hex to print
;   ebx = offset
print_hex_pm:
    pusha

    mov esi, 0xb8000
    shl ebx, 1 ; multiply ebx by 2
    add esi, ebx

    mov ah, 0x0f

    mov al, '0'
    mov [esi], ax
    add esi, 2
    mov al, 'x'
    mov [esi], ax
    add esi, 2

    mov cx, 8 ; set loop for 8 times
.hex_loop_pm:
    rol edx, 4 ; rotate for 8 bit
    mov al, dl ; copy last 2 digit
    and al, 0x0f ; mask off the upper digit
    cmp al, 9
    jle .hex_num_print_pm
    add al, 7 ; add 7 to convert to ascii letter
.hex_num_print_pm:
    add al, '0' ; add '0' to convert to ascii digit
    mov [esi], ax
    add esi, 2
    loop .hex_loop_pm
.hex_end_pm:
    popa
    ret
