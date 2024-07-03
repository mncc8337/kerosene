[org SECOND_STAGE_ADDR]
[bits 16]

jmp second_stage

new_line: db 0xa, 0xd, 0

boot_stage2_msg: db "second stage loaded", 0xd, 0xa, 0
a20_ok_msg: db "A20 enabled", 0xd, 0xa, 0
a20_failed_msg: db "failed to enable A20", 0xd, 0xa, 0

memmap_ok_msg: db " memmap entries detected", 0xd, 0xa, 0
memmap_failed_msg: db "failed to detect memory, system halted", 0

mmap_addr: dw 0

%include "rm/print_string.asm"
%include "rm/print_hex.asm"
%include "rm/disk_load.asm"
%include "rm/a20.asm"
%include "rm/e820_memmap.asm"
%include "switch_to_pm.asm"

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

    ; detect mem
	xor ax, ax
	mov es, ax
	mov edi, MMAP_ADDR
    call e820_memmap
    jnc .memmap_ok

    mov si, memmap_failed_msg
    call print_string
    cli
    hlt

.memmap_ok:
	mov word [MMAP_ENTRY_CNT_ADDR], bp
    mov dx, [MMAP_ENTRY_CNT_ADDR]
    call print_hex
    mov si, memmap_ok_msg
    call print_string

    mov si, new_line
    call print_string

    jmp switch_to_pm

pm_msg: db "switched to protected mode, now loading kernel", 0

%include "pm/print_string_pm.asm"

[bits 32]
BEGIN_PM:

    mov esi, pm_msg
    mov ebx, 400
    call print_string_pm

    ; preparing boot info
    ; entry cnt
    mov edx, [MMAP_ENTRY_CNT_ADDR]
    push edx

    ; memmap entry ptr
    push MMAP_ADDR

    call KERNEL_ADDR
    cli
loopend:                ; infinite loop when finished
    hlt
    jmp loopend

; add padding to fit in 2 sector
times 512 * 2 - ($ - $$) db 0
