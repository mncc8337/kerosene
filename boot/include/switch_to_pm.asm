[bits 16]

%include "gdt.asm"

switch_to_pm:
    cli ; we must switch off interrupts until we have
        ; setup the protected mode interrupt vector
        ; otherwise interrupts will run riot

    lgdt [gdt_descriptor]
    mov eax, cr0 ; to switch to protected mode, we set
    or eax, 0x1  ; the first bit of cr0
    mov cr0, eax

    jmp CODE_SEG: init_pm ; make a far jump (i.e. to a new segment) to our 32 bit codes
                          ; this also forces the CPU to flush its cache of
                          ; prefetched and real mode decoded instructions, which can
                          ; cause problems

[bits 32]

; init registers and the stack once in PM
init_pm:
    mov ax, DATA_SEG ; now in PM, our old segments are meaningless,
    mov ds, ax       ; so we point our segment registers to the
    mov ss, ax       ; data selector we defined in our GDT
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000  ; update the stack position so it is right
    mov esp, ebp      ; at the top of the free space

    call BEGIN_PM     ; finally, call some well-known label

    jmp $
