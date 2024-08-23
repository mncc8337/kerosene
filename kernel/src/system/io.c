#include "system.h"

uint8_t port_inb(uint16_t port) {
    uint8_t result;
    asm volatile("inb %w1, %b0" : "=a" (result) : "Nd" (port));
    return result;
}

void port_outb(uint16_t port, uint8_t data) {
    asm volatile("outb %b0, %w1" : : "a" (data), "Nd" (port));
}

uint16_t port_inw(uint16_t port) {
    uint16_t result;
    asm volatile("inw %w1, %w0" : "=a" (result) : "Nd" (port));
    return result;
}

void port_outw(uint16_t port, uint16_t data) {
    asm volatile("outw %w0, %w1" : : "a" (data), "Nd" (port));
}

void io_wait() {
    port_outb(0x80, 0);
}
