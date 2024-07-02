#pragma once

#define IDT_MAX_DESCRIPTORS 256

#define SYS_SLEEP asm volatile("sti; hlt; cli") 

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    unsigned int gs, fs, es, ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no, err_code;
    unsigned int eip, cs, eflags, useresp, ss; 
} __attribute__((packed)) regs;

// port_io.c
unsigned char port_inb(unsigned short port);
void port_outb(unsigned short port, unsigned char data);
unsigned short port_inw(unsigned short port);
void port_outw(unsigned short port, unsigned short data);
void io_wait();

// gdt.c
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void gdt_init();

// idt.c
void idt_init();
void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags);

// isr.c
void isr_init();

// irq.c
void irq_init();
void irq_install_handler(int irq, void (*handler)(regs *r));
void irq_uninstall_handler(int irq);
