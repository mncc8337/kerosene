#include "system.h"

uint8_t port_inb(uint16_t port) {
    // a handy C wrapper function that reads a byte from the specified port
    // "=a" (result): put AL register in variable RESULT when finished
    // "d" (port): load EDX with port
    uint8_t result;
    asm volatile("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}
void port_outb(uint16_t port, uint8_t data) {
    // "a" ( data ): load EAX with data
    // "d" ( port ): load EDX with port
    asm volatile("out %%al, %%dx" : : "a" (data), "d" (port));
}
uint16_t port_inw(uint16_t port) {
    unsigned short result;
    asm volatile("in %%dx, %%ax" : "=a" (result) : "d" (port));
    return result;
}
void port_outw(uint16_t port, uint16_t data) {
    asm volatile("out %%ax, %%dx" : : "a" (data), "d" (port));
}
void io_wait() {
    port_outb(0x80, 0);
}
