cls:
	pusha
	mov al, 0x3
	mov ah, 0x0
	int 0x10
	popa
	ret

; print message located at the address stored in the bx register
print:
    mov ah, 0xe
    .start_loop:
        mov al, byte [bx]
        cmp al, 0
        je .end_loop

        int 0x10
        inc bx
        jmp .start_loop
    .end_loop:
        ret

%define ENDL 0x0d, 0x0a
%define ENDS 0x0
