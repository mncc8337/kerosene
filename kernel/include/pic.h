#pragma once

#include <stdint.h>

#define PORT_PIC1_COMM  0x20
#define PORT_PIC1_DATA  0x21
#define PORT_PIC2_COMM  0xa0
#define PORT_PIC2_DATA  0xa1

#define PIC_EOI 0x20

#define ICW1_ICW4 0x01
#define ICW1_SINGLE 0x02
#define ICW1_INTERVAL4 0x04
#define ICW1_LEVEL 0x08
#define ICW1_INIT 0x10

#define ICW4_8086 0x01e
#define ICW4_AUTO 0x02
#define ICW4_BUF_SLAVE 0x08
#define ICW4_BUF_MASTER 0x0c
#define ICW4_SFNM 0x10

void pic_send_eoi(uint8_t irq);
void pic_remap(uint8_t offset1, uint8_t offset2);
void pic_disable();
void irq_set_mask(uint8_t irq_line);
void irq_clear_mask(uint8_t irq_line);
