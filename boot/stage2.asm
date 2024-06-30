[org SECOND_STAGE_OFFSET]
[bits 16]

; this will be set in the first stage
BOOT_DRIVE: dw 0

jmp second_stage

boot_stage2_msg: db "successfully landed in second stage", 0xd, 0xa, 0
a20_ok_msg: db "a20 enabled", 0xd, 0xa, 0
a20_failed_msg: db "failed to enable A20", 0xd, 0xa, 0
kernel_loaded_msg: db "kernel loaded", 0xd, 0xa, 0xd, 0xa, 0

%include "print_string.asm"
%include "print_hex.asm"
%include "disk_load.asm"

[bits 16]
second_stage:
    mov si, boot_stage2_msg
    call print_string

    ; enable A20 line
    call enable_a20
    jc .failed_a20
    jmp .success_a20
.failed_a20:
    mov si, a20_failed_msg
    call print_string
    jmp .done_a20
.success_a20:
    mov si, a20_ok_msg
    call print_string
.done_a20:

    ; load kernel
    mov bx, KERNEL_OFFSET
    mov cl, 4 ; kernel start in sector 4
    mov dh, KERNEL_PADDING
    mov dl, [BOOT_DRIVE]
    call disk_load

    mov si, kernel_loaded_msg
    call print_string

    call switch_to_pm

    jmp $

pm_msg: db "switched to protected mode, now loading kernel", 0

%include "a20.asm"
%include "switch_to_pm.asm"
%include "print_string_pm.asm"
%include "print_hex_pm.asm"

[bits 32]
BEGIN_PM:
    mov ebx, 400

    mov esi, pm_msg
    call print_string_pm
    add ebx, 80

    ; now jump to the kernel and never come back
    jmp KERNEL_OFFSET

; add padding to fit in 2 sector
times 512 * 2 - ($ - $$) db 0
