#include "system.h"

unsigned char* memcpy(unsigned char* dest, unsigned char* src, int cnt);
unsigned char* memset(unsigned char* dest, unsigned char val, int cnt);
unsigned short* memsetw(unsigned short* dest, unsigned short cal, int cnt);
int strlen(const char* str);

unsigned char port_inb(unsigned short port) {
    // a handy C wrapper function that reads a byte from the specified port
    // "=a" (result): put AL register in variable RESULT when finished
    // "d" (port): load EDX with port
    unsigned char result;
    __asm__("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}
void port_outb(unsigned short port, unsigned char data) {
    // "a" ( data ): load EAX with data
    // "d" ( port ): load EDX with port
    __asm__("out %%al, %%dx" : : "a" (data), "d" (port));
}
unsigned short port_inw(unsigned short port) {
    unsigned short result;
    __asm__("in %%dx, %%ax" : "=a" (result) : "d" (port));
    return result;
}
void port_outw(unsigned short port, unsigned short data) {
    __asm__("out %%ax, %%dx" : : "a" (data), "d" (port));
}
