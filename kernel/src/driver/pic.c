#include "pic.h"
#include "system.h"

void pic_send_eoi(uint8_t irq) {
    if(irq >= 8)
        port_outb(PORT_PIC2_COMM, PIC_EOI);
    port_outb(PORT_PIC1_COMM, PIC_EOI);
}

void pic_remap(uint8_t offset1, uint8_t offset2) {
    uint8_t a1, a2;

    a1 = port_inb(PORT_PIC1_DATA); // save masks
    a2 = port_inb(PORT_PIC2_DATA);

    port_outb(PORT_PIC1_COMM, ICW1_INIT | ICW1_ICW4); // starts the initialization sequence (in cascade mode)
    io_wait();
    port_outb(PORT_PIC2_COMM, ICW1_INIT | ICW1_ICW4);
    io_wait();
    port_outb(PORT_PIC1_DATA, offset1);
    io_wait();
    port_outb(PORT_PIC2_DATA, offset2);
    io_wait();
    port_outb(PORT_PIC1_DATA, 4);
    io_wait();
    port_outb(PORT_PIC2_DATA, 2);
    io_wait();

    port_outb(PORT_PIC1_DATA, ICW4_8086);
    io_wait();
    port_outb(PORT_PIC2_DATA, ICW4_8086);
    io_wait();

    port_outb(PORT_PIC1_DATA, a1); // restore saved masks.
    port_outb(PORT_PIC2_DATA, a2);
}

void pic_disable() {
    port_outb(PORT_PIC1_DATA, 0xff);
    port_outb(PORT_PIC2_DATA, 0xff);
}

void irq_set_mask(uint8_t irq_line) {
    uint16_t port;
    uint8_t value;

    if(irq_line < 8) {
        port = PORT_PIC1_DATA;
    }
    else {
        port = PORT_PIC2_DATA;
        irq_line -= 8;
    }
    value = port_inb(port) | (1 << irq_line);
    port_outb(port, value);        
}

void irq_clear_mask(uint8_t irq_line) {
    uint16_t port;
    uint8_t value;

    if(irq_line < 8) {
        port = PORT_PIC1_DATA;
    }
    else {
        port = PORT_PIC2_DATA;
        irq_line -= 8;
    }
    value = port_inb(port) & ~(1 << irq_line);
    port_outb(port, value);        
}
