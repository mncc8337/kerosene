#include "ata.h"

static bool ata_pio_identify() {
    // select target device: master
    port_outb(PORT_ATA_PIO_DRI_SELECT, ATA_PIO_DRI_MASTER);
    // set some stuffs
    port_outb(PORT_ATA_PIO_SECTOR_COUNT, 0);
    port_outb(PORT_ATA_PIO_LBA_LO, 0);
    port_outb(PORT_ATA_PIO_LBA_MI, 0);
    port_outb(PORT_ATA_PIO_LBA_HI, 0);
    // send IDENTIFY command
    port_outb(PORT_ATA_PIO_COMM, ATA_PIO_CMD_IDENTIFY);

    // read status port
    // the osdev wiki said you should read it 15 times
    // and only pay attention to the value returned by the last one
    for(int i = 1; i <= 14; i++) port_inb(PORT_ATA_PIO_STAT);
    int stat = port_inb(PORT_ATA_PIO_STAT);
    if(stat == 0) // drive does not exists
        return false;

    // poll until bit 7 (BSY) is clear
    while(stat & 0x80) stat = port_inb(PORT_ATA_PIO_STAT);
    // check if LBA low and LBA high are non-zero
    // if so they are not ATA device
    if(port_inb(PORT_ATA_PIO_LBA_LO) && port_inb(PORT_ATA_PIO_LBA_HI))
        return false;

    return true;
}

bool ata_pio_init(uint16_t* buff) {
    // reset drive
    // port_outb(0x3f6, 0);

    bool dev_available = ata_pio_identify();
    if(!dev_available) return false;

    // continue polling until bit 3 (DRQ) or bit 0 (ERR) is set
    int stat = 0;
    while(stat & 0x8 || stat & 0x1) stat = port_inb(PORT_ATA_PIO_STAT);
    // if bit 0 (ERR) is set then data is not ready
    if(stat & 0x1) return false;

    for(int i = 0; i < 256; i++) buff[i] = port_inw(PORT_ATA_PIO_DATA);
    return true;
}
