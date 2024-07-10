#include "system.h"

unsigned char port_inb(unsigned short port) {
    // a handy C wrapper function that reads a byte from the specified port
    // "=a" (result): put AL register in variable RESULT when finished
    // "d" (port): load EDX with port
    unsigned char result;
    asm volatile("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}
void port_outb(unsigned short port, unsigned char data) {
    // "a" ( data ): load EAX with data
    // "d" ( port ): load EDX with port
    asm volatile("out %%al, %%dx" : : "a" (data), "d" (port));
}
unsigned short port_inw(unsigned short port) {
    unsigned short result;
    asm volatile("in %%dx, %%ax" : "=a" (result) : "d" (port));
    return result;
}
void port_outw(unsigned short port, unsigned short data) {
    asm volatile("out %%ax, %%dx" : : "a" (data), "d" (port));
}
void io_wait() {
    port_outb(0x80, 0);
}
