#pragma once

#define IDT_MAX_DESCRIPTORS 256

#define SYS_SLEEP asm volatile("sti; hlt; cli") 

// no bool in plain C so i make one
typedef enum {false, true} bool;

struct regs {
    unsigned int gs, fs, es, ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no, err_code;
    unsigned int eip, cs, eflags, useresp, ss; 
} __attribute__((packed));

// string.h
int string_len(char* str);
char* to_string(int num);
bool string_cmp(char* str1, char* str2);
char* substr(char* str, unsigned int p1, unsigned int p2);
void tokenize(char* string, char sep, int*, int* token_count);

// port_io.c
unsigned char port_inb(unsigned short port);
void port_outb(unsigned short port, unsigned char data);
unsigned short port_inw(unsigned short port);
void port_outw(unsigned short port, unsigned short data);
void io_wait();

// gdt.c
void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran);
void gdt_init();

// idt.c
void idt_init();
void idt_set_descriptor(unsigned char vector, void* isr, unsigned char flags);

// isr.c
void isr_init();

// irq.c
void irq_init();
void irq_install_handler(int irq, void (*handler)(struct regs *r));
void irq_uninstall_handler(int irq);
