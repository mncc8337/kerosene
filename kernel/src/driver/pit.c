#include "pit.h"
#include "system.h"

void pit_timer_frequency(int hz) {
    uint16_t divisor = 1193180 / hz; // this number will later be the divisor of 1193180
    port_outb(PORT_PIT_COM, 0x36);
    port_outb(PORT_PIT_CH0, divisor & 0xff);
    port_outb(PORT_PIT_CH0, divisor >> 8);
}
