[bits 32]

global enter_usermode
enter_usermode:
    cli

    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp
    push 0x23
    push eax
    pushf

    ; enable interrupt flag
    pop eax
    or eax, 0x200
    push eax

    push 0x1b
    lea eax, [usr_start]
    push eax

    iret
usr_start:
    add esp, 4
    ret
