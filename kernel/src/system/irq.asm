[bits 32]

%define DATA_SEG 0x10

%assign i 0 
%rep 16
irq_stub_%+i:
    cli
    push byte 0
    push byte i+32
    jmp irq_common_stub
%assign i i+1 
%endrep

extern irq_handler
irq_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp
    push eax
    mov eax, irq_handler
    call eax
    pop eax
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret

global irq_table
irq_table:
%assign i 0 
%rep 16
    dd irq_stub_%+i
%assign i i+1 
%endrep
