#include "system.h"

struct idt_entry {
    unsigned short    isr_low;      // the lower 16 bits of the ISR's address
    unsigned short    kernel_cs;    // the GDT segment selector that the CPU will load into CS before calling the ISR
    unsigned char     reserved;     // set to zero
    unsigned char     attributes;   // type and attributes; see the IDT page
    unsigned short    isr_high;     // the higher 16 bits of the ISR's address
} __attribute__((packed));

struct idtr {
    unsigned short  limit;
    unsigned int    base;
} __attribute__((packed));

__attribute__((aligned(0x10))) 
struct idt_entry idt[IDT_MAX_DESCRIPTORS]; // create an array of IDT entries; aligned for performance
struct idtr idtr;

bool vectors[IDT_MAX_DESCRIPTORS];

void idt_set_descriptor(unsigned char vector, void* isr, unsigned char flags) {
    struct idt_entry* descriptor = &idt[vector];

    descriptor->isr_low    = (unsigned int)isr & 0xffff;
    descriptor->kernel_cs  = 0x08;
    descriptor->attributes = flags;
    descriptor->isr_high   = (unsigned int)isr >> 16;
    descriptor->reserved   = 0;

    vectors[vector] = true;
}

extern void load_idt();

void idt_init() {
    idtr.base = (unsigned int)&idt[0];
    idtr.limit = (unsigned short)sizeof(struct idt_entry) * IDT_MAX_DESCRIPTORS - 1;

    isr_init();

    load_idt();
}
