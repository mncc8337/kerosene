#include "pic.h"
#include "system.h"

void pic_send_eoi(unsigned char irq) {
    if(irq >= 8)
        port_outb(PORT_PIC2_COMM, PIC_EOI);
    port_outb(PORT_PIC1_COMM, PIC_EOI);
}

void pic_remap(int offset1, int offset2) {
    unsigned char a1, a2;

    a1 = port_inb(PORT_PIC1_DATA);                        // save masks
    a2 = port_inb(PORT_PIC2_DATA);

    port_outb(PORT_PIC1_COMM, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
    io_wait();
    port_outb(PORT_PIC2_COMM, ICW1_INIT | ICW1_ICW4);
    io_wait();
    port_outb(PORT_PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
    io_wait();
    port_outb(PORT_PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
    io_wait();
    port_outb(PORT_PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    io_wait();
    port_outb(PORT_PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    port_outb(PORT_PIC1_DATA, ICW4_8086);               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
    io_wait();
    port_outb(PORT_PIC2_DATA, ICW4_8086);
    io_wait();

    port_outb(PORT_PIC1_DATA, a1);   // restore saved masks.
    port_outb(PORT_PIC2_DATA, a2);
}

void pic_disable() {
    port_outb(PORT_PIC1_DATA, 0xff);
    port_outb(PORT_PIC2_DATA, 0xff);
}

void irq_set_mask(unsigned char irq_line) {
    unsigned short port;
    unsigned char value;

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

void irq_clear_mask(unsigned char irq_line) {
    unsigned short port;
    unsigned char value;

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
