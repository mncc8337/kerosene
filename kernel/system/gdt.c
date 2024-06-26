#include "system.h"

// add another GDT that can be managed by the kernel

// prevent compiler optimization by adding attr packed
struct gdt_entry {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char base_middle;
    unsigned char access;
    unsigned char granularity;
    unsigned char base_high;
} __attribute__((packed));

// special pointer which includes the limit: the max bytes taken up by the GDT, minus 1
struct gdt_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

struct gdt_entry gdt[3];
struct gdt_ptr gp;

// will exist in kernel_entry.asm to reload new segment registers
extern void gdt_flush();

void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran) {
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
    gp.limit = sizeof(struct gdt_entry) * 3 - 1;
    gp.base = (int)&gdt;

    // NULL descriptor
    gdt_set_gate(0, 0, 0, 0, 0);

    // code segment
    gdt_set_gate(1, 0, 0xffffffff, 0x9a, 0xcf);

    // data segment
    gdt_set_gate(2, 0, 0xffffffff, 0x92, 0xcf);

    // flush out the old GDT and install the new changes
    gdt_flush();
}
