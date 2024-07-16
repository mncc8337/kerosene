[bits 32]

%define CODE_SEG 0x08
%define DATA_SEG 0x10

global gdt_flush
extern gdtr
gdt_flush:
    lgdt [gdtr]
    jmp CODE_SEG:.flush ; far jump to flush all caches
.flush:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret
