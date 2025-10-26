#include "system.h"

static gdt_entry_t gdt[GDT_MAX_DESCRIPTORS];
gdtr_t gdtr;

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = base & 0xffff;
    gdt[num].base_middle = (base >> 16) & 0xff;
    gdt[num].base_high = (base >> 24) & 0xff;

    gdt[num].limit_low = limit & 0xffff;
    gdt[num].granularity = (limit >> 16) & 0xf;

    gdt[num].granularity |= (gran << 4) & 0xf0;
    gdt[num].access = access;
}

// from gdt.asm
extern void gdt_flush();

void gdt_init() {
    // setup the GDT pointer and limit
    gdtr.limit = sizeof(gdt_entry_t) * GDT_MAX_DESCRIPTORS - 1;
    gdtr.base = (unsigned)&gdt;
    
    // NULL descriptor
    gdt_set_gate(0, 0, 0, 0, 0);
    // ring0 code segment
    gdt_set_gate(1, 0, 0xfffff, 0x9a, 0xc);
    // ring0 data segment
    gdt_set_gate(2, 0, 0xfffff, 0x92, 0xc);
    // ring3 code segment
    gdt_set_gate(3, 0, 0xfffff, 0xfa, 0xc);
    // ring3 data segment
    gdt_set_gate(4, 0, 0xfffff, 0xf2, 0xc);
    // TSS
    tss_install(5, 0x10, 0);

    gdt_flush();
    tss_flush();
}
