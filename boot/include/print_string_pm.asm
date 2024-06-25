[bits 32]

; Define some constants
VIDEO_MEMORY equ 0xb8000
WHITE_ON_BLACK equ 0x0f

; print string pointed by esi
print_string_pm:
    pusha
    mov edx, VIDEO_MEMORY ; Set edx to the start of vid mem.
.print_pm:
    lodsb ; load next character into al
    cmp al, 0
    je .print_pm_done

    mov ah, WHITE_ON_BLACK ; store attr in ah
    mov [edx], ax ; set character and its attr
    add edx, 2    ; move to next character cell in vid mem
    jmp .print_pm
.print_pm_done:
    popa
    ret
    
