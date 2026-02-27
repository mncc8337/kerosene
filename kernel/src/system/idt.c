#include <system.h>

static idt_entry_t idt[IDT_MAX_DESCRIPTORS];
idtr_t idtr;

static bool vectors[IDT_MAX_DESCRIPTORS];

void idt_set_descriptor(uint8_t vector, uint32_t (*isr)(regs_t*), uint8_t flags) {
    idt_entry_t* descriptor = &idt[vector];

    descriptor->isr_low    = (uint32_t)isr & 0xffff;
    descriptor->kernel_cs  = 0x08;
    descriptor->attributes = flags;
    descriptor->isr_high   = (uint32_t)isr >> 16;
    descriptor->reserved   = 0;

    vectors[vector] = true;
}

void idt_init() {
    idtr.base = (uint32_t)idt;
    idtr.limit = sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

    asm("lidt %0" : : "m" (idtr));
}
