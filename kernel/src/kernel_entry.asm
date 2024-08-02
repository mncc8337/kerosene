; see https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
; for more infomation

MBALIGN  equ  1 << 0 ; align loaded modules on page boundaries
MEMINFO  equ  1 << 1 ; provide memory map
VIDMODE  equ  1 << 2 ; video mode table
ADDRESS  equ  1 << 16 ; custom image loading location
MBFLAGS  equ  MBALIGN | MEMINFO | VIDMODE
MAGIC    equ  0x1BADB002
CHECKSUM equ -(MAGIC + MBFLAGS)

section .multiboot
align 4
	dd MAGIC
	dd MBFLAGS
	dd CHECKSUM
    dd 0 ; header addr
    dd 0 ; load addr
    dd 0 ; load end addr
    dd 0 ; bss end addr
    dd 0 ; entry addr
    dd 1  ; video mode. 0 for graphics mode and 1 for text mode
    dd 80 ; video width
    dd 25 ; video height
    dd 0  ; video depth

section .bss
align 16
kernel_stack_bottom:
resb 16384 ; 16 KiB for kernel stack
kernel_stack_top:

section .text
global _start: function (_start.end - _start)
_start:
	mov esp, kernel_stack_top
    push eax ; magic value
    push ebx ; multiboot infomation structure
    extern kmain
	call kmain

.hang:
    cli
    hlt
	jmp .hang

.end:
