[bits 32]
CODE_SEG equ 0x08
DATA_SEG equ 0x10

; jump straight to main after loaded the kernel
; skip all codes prior to main
extern main
jmp main

;;;;;;;;;;;;;;;;;

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

; IDT
global load_idt
extern idtr
load_idt:
    lidt [idtr]
    ret

; ISR
%macro isr_err_stub 1
isr_stub_%1:
    cli
    push byte %1
    jmp isr_common_stub
%endmacro

%macro isr_no_err_stub 1
isr_stub_%1:
    cli
    push byte 0
    push byte %1
    jmp isr_common_stub
%endmacro

isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_err_stub    21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_err_stub    29
isr_err_stub    30
isr_no_err_stub 31

extern exception_handler
isr_common_stub:
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
    mov eax, esp   ; push us the stack
    push eax
    mov eax, exception_handler
    call eax       ; a special call, preserves the 'eip' register
    pop eax
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8     ; clean up the pushed error code and pushed ISR number
    iret           ; interrupt return

global isr_table
isr_table:
%assign i 0
%rep 32
    dd isr_stub_%+i
%assign i i+1
%endrep

; IRQ
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
