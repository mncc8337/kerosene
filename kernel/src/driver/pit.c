#include "pit.h"
#include "system.h"

void pit_timer_phase(int hz) {
    unsigned short divisor = 1193180 / hz; // this number will later be the divisor of 1193180
    port_outb(PORT_PIT_COMM, 0x36);
    port_outb(PORT_PIT0_DATA, divisor & 0xff);   // Set low byte of divisor
    port_outb(PORT_PIT0_DATA, divisor >> 8);     // Set high byte of divisor
}
