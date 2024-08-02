#include "system.h"
#include "kpanic.h"
#include "tty.h"
#include "pic.h"
#include "string.h"

static void* routines[IDT_MAX_DESCRIPTORS];
// from isr.asm
extern void* isr_table[IDT_MAX_DESCRIPTORS];

static char* exception_message[] = {
    "Division Error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved"
};

// default exception handler
static void exception_handler(regs_t* r) {
    tty_print_string("Exception: ", -1, WHITE, true);
    tty_print_string(exception_message[r->int_no], -1, LIGHT_RED, true);
    tty_print_string(".\nSystem Halted!", -1, WHITE, true);
    kpanic();
}

// default ISR. every interrupt will be "handled" by this function
void isr_handler(regs_t* reg) {
    void (*handler)(regs_t*);

    handler = routines[reg->int_no];
    if(handler) handler(reg);

    // if is an IRQ
    if(reg->int_no >= 32 && reg->int_no <= 47)
        pic_send_eoi(reg->int_no - 32);
}

void irq_install_handler(int irq, void (*handler)(regs_t* r)) {
    if(irq > 15) return;
    routines[irq+32] = handler;
}
void irq_uninstall_handler(int irq) {
    if(irq > 15) return;
    routines[irq+32] = 0;
}

void isr_new_interrupt(int isr, uint8_t flags, void (*handler)(regs_t* r)) {
    idt_set_descriptor(isr, isr_table[isr], flags);
    routines[isr] = handler;
}

void isr_init() {
    // set exception handler
    for(unsigned char vector = 0; vector < 32; vector++) {
        idt_set_descriptor(vector, isr_table[vector], 0x8e);
        routines[vector] = exception_handler;
    }

    // set IRQ handler
    // send commands to the PIC to make IRQ0 to 15 be remapped to IDT entry 32 to 47
    pic_remap(32, 40); // master holds 8 entries so slave is mapped to 40
    for(int vector = 32; vector < 48; vector++)
        idt_set_descriptor(vector, isr_table[vector], 0x8e);

    memset(routines + 32, 0, 223);
}

