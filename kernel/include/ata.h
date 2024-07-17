#pragma once

#include "stdbool.h"
#include "stdint.h"

#include "system.h"

// idk the difference between primary bus and secondary bus
// so i only use the primary bus here

#define PORT_ATA_PIO_DATA 0x1f0
#define PORT_ATA_PIO_STAT 0x1f7
#define PORT_ATA_PIO_COMM 0x1f7

#define PORT_ATA_PIO_CTRL 0x3f6

#define PORT_ATA_PIO_DRI_SELECT 0xf6

#define PORT_ATA_PIO_SECTOR_COUNT 0x1f2
#define PORT_ATA_PIO_LBA_LO 0x1f3
#define PORT_ATA_PIO_LBA_MI 0x1f4
#define PORT_ATA_PIO_LBA_HI 0x1f5

#define ATA_PIO_DRI_MASTER 0xa0
#define ATA_PIO_DRI_SLAVE  0xb0

#define ATA_PIO_CMD_IDENTIFY 0xec

// ata_pio.c
bool ata_pio_init(uint16_t* buff);
