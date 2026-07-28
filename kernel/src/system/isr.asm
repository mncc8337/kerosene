[bits 32]

%macro isr_err_stub 1
isr_stub_%1:
    push %1
    jmp isr_common_stub
%endmacro

%macro isr_no_err_stub 1
isr_stub_%1:
    push byte 0 ; this is a dummy error code
    push %1
    jmp isr_common_stub
%endmacro

; exceptions
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
; IRQ 0 to 15
isr_no_err_stub 32
isr_no_err_stub 33
isr_no_err_stub 34
isr_no_err_stub 35
isr_no_err_stub 36
isr_no_err_stub 37
isr_no_err_stub 38
isr_no_err_stub 39
isr_no_err_stub 40
isr_no_err_stub 41
isr_no_err_stub 42
isr_no_err_stub 43
isr_no_err_stub 44
isr_no_err_stub 45
isr_no_err_stub 46
isr_no_err_stub 47
; empty stubs from 48 to 255, reserved for future
%assign i 48
%rep (256 - 48)
    isr_no_err_stub i
%assign i i+1
%endrep

extern isr_handler ; from isr.c
isr_common_stub:
    ; once interrupted, the cpu has already pushed most of the registers to the kernel stack
    ; we only need to push ds, es, fs, gs and esp to support the kernel's context switching
    ; mechanism (see the commented block below)
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10 ; data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp ; push esp by copy it to eax
    push eax

    mov eax, isr_handler
    cli
    cld
    call eax
    ; return value (next context's esp) now stored on eax

    mov esp, eax ; switch context by switching esp
    ; because processes has already save their registers to their kernel stack
    ; context switching is simply choosing the kernel stack to use
    ; that is why the isr_handler must return the esp of the next context
    ; (or just return the old esp if no context switching is taken place)
    ; the only thing left to do is faking storing registers for new processes
    ; (see process/process.c process_new())

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8 ; clean up error code and isr number
    iret

global isr_table
isr_table:
%assign i 0
%rep 256
    dd isr_stub_%+i
%assign i i+1
%endrep
