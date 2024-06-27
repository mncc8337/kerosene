#include "system.h"
#include "driver/pic.h"

void *irq_routines[16] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

// install custom handler
void irq_install_handler(int irq, void (*handler)(struct regs *r)) {
    irq_routines[irq] = handler;
}
void irq_uninstall_handler(int irq) {
    irq_routines[irq] = 0;
}


extern void* irq_table[];
void irq_init() {
    // send commands to the PIC to make IRQ0 to 15 be remapped to IDT entry 32 to 47
    pic_remap(32, 40); // master holds 8 entries so slave is mapped to 40
    for(int vector = 0; vector < 16; vector++)
        idt_set_descriptor(vector + 32, irq_table[vector], 0x8e);
}

void irq_handler(struct regs *r) {
    // blank function
    void (*handler)(struct regs *r);

    handler = irq_routines[r->int_no - 32];
    if(handler)
        handler(r);

    pic_send_eoi(r->int_no);
}
