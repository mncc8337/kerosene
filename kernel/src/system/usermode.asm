[bits 32]

global enter_usermode
enter_usermode:
    mov ax, (4 * 8) | 3
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push (4 * 8) | 3
    mov eax, esp
    push eax
    pushfd
    push (3 * 8) | 3
    lea eax, [usr_start]
    push eax
    iretd
usr_start:
    add esp, 4
    ret
