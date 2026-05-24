ENTRY(kernel_entry)

SECTIONS {
    . = 0xc0000000 + 2M;

    kernel_start = .;

    .text ALIGN(4K) : AT(ADDR(.text) - 0xc0000000) {
        kernel_text_start = .;
        *(.multiboot)
        *(.text)
        kernel_text_end = .;
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xc0000000) {
        kernel_rodata_start = .;
        *(.rodata)
        kernel_rodata_end = .;
    }

    .data ALIGN(4K) : AT(ADDR(.data) - 0xc0000000) {
        *(.data)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xc0000000) {
        *(COMMON)
        *(.bss)
    }

    kernel_end = .;
}
