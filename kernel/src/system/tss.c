#include "system.h"
#include "string.h"

static tss_entry_t tss;

void tss_set_stack(uint16_t kernel_ss, uint16_t kernel_esp) {
    tss.ss0 = kernel_ss;
    tss.esp0 = kernel_esp;
}

extern void tss_flush();

void tss_install(uint16_t kernel_ss, uint16_t kernel_esp) {
    gdt_set_gate(5, (uint32_t)(&tss), sizeof(tss_entry_t)-1, 0x89, 0x0);

    memset((void*)&tss, 0, sizeof(tss_entry_t));

    tss_set_stack(kernel_ss, kernel_esp);
    // tss.cs = 0x0b;
    // tss.ss = 0x13;
    // tss.es = 0x13;
    // tss.ds = 0x13;
    // tss.fs = 0x13;
    // tss.gs = 0x13;

    tss_flush();
}
