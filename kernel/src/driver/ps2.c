#include "ps2.h"
#include "system.h"

uint8_t ps2_read_data() {
    return port_inb(PORT_PS2_DATA);
}
void ps2_write_data(uint8_t dat) {
    return port_outb(PORT_PS2_DATA, dat);
}
uint8_t ps2_read_stat() {
    return port_inb(PORT_PS2_STAT);
}

void ps2_wait_for_reading_data() {
    while(!(port_inb(PORT_PS2_STAT) & 1));
}
void ps2_wait_for_writing_data() {
    while(port_inb(PORT_PS2_STAT) & 0x2);
}

void ps2_read_from_2nd_port() {
    // TODO: turn 0xd4 into a macro
    port_outb(PORT_PS2_COMM, 0xd4);
}
