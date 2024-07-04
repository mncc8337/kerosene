#include "system.h"

typedef struct {
    uint16_t isr_low;      // the lower 16 bits of the ISR's address
    uint16_t kernel_cs;    // the GDT segment selector that the CPU will load into CS before calling the ISR
    uint8_t  reserved;     // set to zero
    uint8_t  attributes;   // type and attributes; see the IDT page
    uint16_t isr_high;     // the higher 16 bits of the ISR's address
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idtr_t;

__attribute__((aligned(0x10))) 
static idt_entry_t idt[IDT_MAX_DESCRIPTORS]; // create an array of IDT entries; aligned for performance
idtr_t idtr;

static bool vectors[IDT_MAX_DESCRIPTORS];

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    idt_entry_t* descriptor = &idt[vector];

    descriptor->isr_low    = (uint32_t)isr & 0xffff;
    descriptor->kernel_cs  = 0x08;
    descriptor->attributes = flags;
    descriptor->isr_high   = (uint32_t)isr >> 16;
    descriptor->reserved   = 0;

    vectors[vector] = true;
}

extern void load_idt();

void idt_init() {
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

    isr_init();

    load_idt();
}
