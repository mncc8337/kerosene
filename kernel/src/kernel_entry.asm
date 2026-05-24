; see https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
; for more infomation

MBALIGN  equ  1 << 0 ; align loaded modules on page boundaries
MEMINFO  equ  1 << 1 ; provide memory map
VIDMODE  equ  1 << 2 ; video mode table
ADDRESS  equ  1 << 16 ; custom image loading location
MBFLAGS  equ  MBALIGN | MEMINFO | VIDMODE
MAGIC    equ  0x1BADB002
CHECKSUM equ -(MAGIC + MBFLAGS)

%define phys_to_virt(phys) (((phys) >> 22) * 4)

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
; early kernel stack
global kernel_stack_bottom
global kernel_stack_top
kernel_stack_bottom:
resb 4 * 1024
kernel_stack_top:

section .data
multiboot_ptr: dd 0
align 4096
global kernel_pd_data
kernel_pd_data:
    times 1024 dd 0x00000000
page_table_4mib:
    times 1024 dd 0x00000000
page_table_kernel1: ; extend if kernel exceed 2MiB
    times 1024 dd 0x00000000

section .text
global kernel_entry
kernel_entry:
    cmp eax, 0x2BADB002 ; check multiboot magic
    jne hang ;  hang if invalid
    mov [multiboot_ptr - KERNEL_START], ebx ; save multiboot ptr
    ; now eax and ebx are free to use

    ; add page tables to page directory

    ; recursive paging
    ; by putting the page directory address in the last page directory entry
    ; the system will process the page directory as a page table
    ; thus mapping all the page table address for us
    ; isn't it genius?
    mov eax, kernel_pd_data - KERNEL_START
    or eax, 0b011
    mov [kernel_pd_data - KERNEL_START + 1023 * 4], eax

    mov eax, page_table_4mib - KERNEL_START
    or eax, 0b011
    mov [kernel_pd_data - KERNEL_START + phys_to_virt(0)], eax

    mov eax, page_table_kernel1 - KERNEL_START
    or eax, 0b011
    mov [kernel_pd_data - KERNEL_START + phys_to_virt(KERNEL_START)], eax

    ; identity map the first 4MiB
    mov edi, page_table_4mib - KERNEL_START
    mov eax, 0x0
    or eax, 0b011
    mov ecx, 1024
.loop_4mib:
    mov [edi], eax
    add edi, 4
    add eax, 4096
    loop .loop_4mib

    ; map the first 4MiB to 0xc0000000
    ; this includes BIOS/GRUB stuff (first 2MiB)
    ; and our kernel (next 2MiB)
    mov edi, page_table_kernel1 - KERNEL_START
    mov eax, 0x0
    ; temporary map all kernel data as rw
    ; upon reaching kinit() we will set
    ; the correct permission for each part (text and rodata)
    or eax, 0b011
    mov ecx, 1024
.loop_kernel1:
    mov [edi], eax
    add edi, 4
    add eax, 4096
    loop .loop_kernel1

    ; note that we dont need to map kernel_pd_data
    ; since we had recursively mapped it (see above code)
    ; and is always accessible though VMMNGR_PD (0xfffff000) address

    ; load page directory
    mov eax, kernel_pd_data - KERNEL_START
    mov cr3, eax

    ; enable paging
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; now jump
    lea eax, [higher_half]
    jmp eax

higher_half:
    ; unmap the first 4MiB identity
    mov dword [kernel_pd_data], 0x0
    mov eax, cr3
    mov cr3, eax

    mov esp, kernel_stack_top
    push dword [multiboot_ptr]
    xor ebp, ebp
    extern kinit
    call kinit

hang:
    cli
    hlt
    jmp hang
end:
