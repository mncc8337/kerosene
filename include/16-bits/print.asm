%define ENDL 0xd, 0xa

cls:
    push ax

	mov al, 0x3
	mov ah, 0x0
	int 0x10

    pop ax
	ret

; print string located at the address stored in si
print_string:
    mov ah, 0xe
.print:
    lodsb
    cmp al, 0x0
    je .end_print

    int 0x10
    jmp .print
.end_print:
    ret

print_new_line:
    mov ah, 0xe
    mov al, 0xd
    int 0x10
    mov al, 0xa
    int 0x10
    ret

; print hex value in dx
print_hex:
    pusha
    mov ah, 0xe

    ; print the '0x' part
    mov al, '0'
    int 0x10
    mov al, 'x'
    int 0x10

    mov cx, 4 ; set loop for  4 times
.hex_loop:
    rol dx, 4 ; rotate for 4 bit
    mov al, dl ; copy last 2 digit
    and al, 0x0f ; mask off the upper digit
    cmp al, 9
    jle .hex_num_print
    add al, 7 ; add 7 to convert to ascii letter
.hex_num_print:
    add al, '0' ; add '0' to convert to ascii digit
    int 0x10 ; now print
    loop .hex_loop
.hex_end:
    popa
    ret
