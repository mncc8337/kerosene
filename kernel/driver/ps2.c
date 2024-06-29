#include "driver/ps2.h"

unsigned char ps2_read_data() {
    return port_inb(PORT_PS2_DATA);
}
void ps2_write_data(unsigned char dat) {
    return port_outb(PORT_PS2_DATA, dat);
}
unsigned char ps2_read_stat() {
    return port_inb(PORT_PS2_STAT);
}

void ps2_wait_for_reading_data() {
    unsigned char stat = port_inb(PORT_PS2_STAT);
    bool output_buffer_full = stat & 1;

    while(!output_buffer_full) {
        stat = port_inb(PORT_PS2_STAT);
        output_buffer_full = stat & 1; // get the last bit
    }
}
void ps2_wait_for_writing_data() {
    unsigned char stat = port_inb(PORT_PS2_STAT) >> 1;
    bool input_buffer_empty = stat & 1;

    while(!input_buffer_empty) {
        stat = port_inb(PORT_PS2_STAT) >> 1;
        input_buffer_empty = stat & 1; // get the last bit
    }
}
