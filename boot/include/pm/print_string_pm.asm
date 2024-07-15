[bits 32]

; in
;   esi = string to print
;   ebx = offset
print_string_pm:
    pusha
    mov edx, 0xb8000 ; Set edx to the start of vid mem.
    shl ebx, 1 ; multiply ebx by 2
    add edx, ebx

    mov ah, 0x07 ; set attr
.print_pm:
    lodsb ; load next character into al
    cmp al, 0
    je .print_pm_done

    mov [edx], ax ; set character and its attr
    add edx, 2    ; move to next character cell in vid mem
    jmp .print_pm
.print_pm_done:
    popa
    ret
    
