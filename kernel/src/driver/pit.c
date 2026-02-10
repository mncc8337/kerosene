#include <pit.h>
#include <system.h>

void pit_timer_frequency(int hz) {
    uint16_t div = 1193180 / hz;
    port_outb(PORT_PIT_COM, 0x36);
    port_outb(PORT_PIT_CH0, div & 0xff);
    port_outb(PORT_PIT_CH0, div >> 8);
}

unsigned pit_get_count() {
    unsigned count = 0;

    asm volatile("cli");

    port_outb(PORT_PIT_COM,0);

    count = port_inb(PORT_PIT_CH0);
    count |= port_inb(PORT_PIT_CH0) << 8;

    asm volatile("sti");

    return count;
}

void pit_set_count(unsigned count) {
    asm volatile("cli");
	
	port_outb(PORT_PIT_CH0, count & 0xff);
	port_outb(PORT_PIT_CH0, (count & 0xff00) >> 8);

    asm volatile("sti");

	return;
}

void pit_beep_start() {
    uint8_t tmp = port_inb(PORT_PC_SPEAKER);
    port_outb(PORT_PC_SPEAKER, tmp | 3);
}

void pit_beep_stop() {
    uint8_t tmp = port_inb(PORT_PC_SPEAKER) & 0xfc;
    port_outb(PORT_PC_SPEAKER, tmp);
}

void pit_beep(int freq) {
    uint16_t div;

    div = 1193180 / freq;
    port_outb(PORT_PIT_COM, 0xb6);
    port_outb(PORT_PIT_CH2, (uint8_t)(div));
    port_outb(PORT_PIT_CH2, (uint8_t)(div >> 8));
}
