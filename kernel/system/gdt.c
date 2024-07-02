#include "system.h"

// add another GDT that can be managed by the kernel

// prevent compiler optimization by adding attr packed
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

// special pointer which includes the limit: the max bytes taken up by the GDT, minus 1
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdtr_t;

gdt_entry_t gdt[3];
gdtr_t gdtr;

// will exist in kernel_entry.asm to reload new segment registers
extern void gdt_flush();

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    // setup the descriptor base address
    gdt[num].base_low = base & 0xffff;
    gdt[num].base_middle = (base >> 16) & 0xff;
    gdt[num].base_high = (base >> 24) & 0xff;

    // setup the descriptor limits
    gdt[num].limit_low = limit & 0xffff;
    gdt[num].granularity = (limit >> 16) & 0x0f;

    // set up the granularity and access flags
    gdt[num].granularity |= gran & 0xf0;
    gdt[num].access = access;
}

void gdt_init() {
    // setup the GDT pointer and limit
    gdtr.limit = sizeof(gdt_entry_t) * 3 - 1;
    gdtr.base = (int)&gdt;

    // NULL descriptor
    gdt_set_gate(0, 0, 0, 0, 0);

    // code segment
    gdt_set_gate(1, 0, 0xffffffff, 0x9a, 0xcf);

    // data segment
    gdt_set_gate(2, 0, 0xffffffff, 0x92, 0xcf);

    // flush out the old GDT and install the new changes
    gdt_flush();
}
