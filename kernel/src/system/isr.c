#include "system.h"
#include "tty.h"

static char *exception_message[] = {
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

void exception_handler(regs* r) {
    tty_print_string("Exception: ", -1, WHITE, true);
    tty_print_string(exception_message[r->int_no], -1, LIGHT_RED, true);
    tty_print_string(".\nSystem Halted!", -1, WHITE, true);
    SYS_HALT(); 
}

extern void* isr_table[];

void isr_init() {
    for(unsigned char vector = 0; vector < 32; vector++)
        idt_set_descriptor(vector, isr_table[vector], 0x8e);
}
