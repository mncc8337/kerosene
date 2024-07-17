#pragma once

#include "system.h"

#define PORT_PIC1_COMM  0x20
#define PORT_PIC1_DATA  0x21
#define PORT_PIC2_COMM  0xa0
#define PORT_PIC2_DATA  0xa1

#define PIC_EOI 0x20

#define ICW1_ICW4 0x01      // indicates that ICW4 will be present
#define ICW1_SINGLE 0x02    // single (cascade) mode
#define ICW1_INTERVAL4 0x04 // call address interval 4 (8)
#define ICW1_LEVEL 0x08     // level triggered (edge) mode
#define ICW1_INIT 0x10      // initialization - required!

#define ICW4_8086 0x01       // 8086/88 (MCS-80/85) mode
#define ICW4_AUTO 0x02       // auto (normal) EOI
#define ICW4_BUF_SLAVE 0x08  // buffered mode/slave
#define ICW4_BUF_MASTER 0x0C // buffered mode/master
#define ICW4_SFNM 0x10       // special fully nested (not)

void pic_send_eoi(unsigned char irq);
void pic_remap(int offset1, int offset2);
void pic_disable();
void irq_set_mask(unsigned char irq_line);
void irq_clear_mask(unsigned char irq_line);
