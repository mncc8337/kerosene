; see https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
; for more infomation

MBALIGN  equ  1 << 0 ; align loaded modules on page boundaries
MEMINFO  equ  1 << 1 ; provide memory map
VIDMODE  equ  1 << 2 ; video mode table
ADDRESS  equ  1 << 16 ; custom image loading location
MBFLAGS  equ  MBALIGN | MEMINFO | VIDMODE
MAGIC    equ  0x1BADB002
CHECKSUM equ -(MAGIC + MBFLAGS)

; virtual memory of kernel
KVMBASE equ 0xc0000000

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
    dd 0 ; 0 for linear graphics mode, 1 for text mode
    dd 1280 ; video width
    dd 720 ; video height
    dd 32  ; video depth

section .bss
; kernel stack
kernel_stack_bottom:
resb 16384 ; 16 KiB
kernel_stack_top:

section .data
align 4096
boot_page_directory:
    times 1024 dd 0x00000002
boot_page_table_4mb:
    times 1024 dd 0x00000000
boot_page_table_kernel1: ; extend if exceed 4MiB
    times 1024 dd 0x00000000

section .text
global kernel_entry
kernel_entry:
    ; identity map first 4mb
    mov ecx, boot_page_table_4mb - KVMBASE
    or ecx, 3 ; supervisor level, read/write, present
    mov [boot_page_directory - KVMBASE], ecx

    mov esi, boot_page_table_4mb - KVMBASE
    mov ecx, 0x0 + 0b011 ; start at 0, supervisor level, read/write, present
.loop_4mb:
    mov [esi], ecx
    add esi, 4
    add ecx, 4096
    cmp esi, boot_page_table_4mb - KVMBASE + 4096
    jb .loop_4mb

    ; map kernel to 0xc0000000
    mov ecx, boot_page_table_kernel1 - KVMBASE
    or ecx, 3 ; supervisor level, read/write, present
    mov [boot_page_directory - KVMBASE + (KVMBASE >> 22) * 4], ecx

    mov esi, boot_page_table_kernel1 - KVMBASE
    mov ecx, 0x0 + 0b011 ; start at 0, supervisor level, read/write, present
.loop_kernel1:
    mov [esi], ecx
    add esi, 4
    add ecx, 4096
    cmp esi, boot_page_table_kernel1 - KVMBASE + 4096
    jb .loop_kernel1

    ; load page directory
    mov ecx, boot_page_directory - KVMBASE
    mov cr3, ecx

    ; enable paging
    mov ecx, cr0
    or ecx, 0x80000000
    mov cr0, ecx

    ; now jump
    lea ecx, [higher_half]
    jmp ecx

higher_half:
    ; TODO: map video addr before entering the kernel

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
