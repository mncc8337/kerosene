#include "ata.h"

static bool LBA28_mode;
static bool LBA48_mode;
static uint8_t supported_UDMA;
static uint8_t active_UDMA;
static bool cable80;
static uint32_t total_addressable_sec_LBA28;
static uint64_t total_addressable_sec_LBA48;

static char* error_msg[] = {
    "AMNF - Address mark not found",
    "TKZNF - Track zero not found",
    "ABRT - Aborted command",
    "MCR - Media change request",
    "IDNF - ID not found",
    "MC - Media changed",
    "UNC - Uncorrectable data error",
    "BBK - Bad Block detected"
};

static bool ata_pio_identify() {
    // select target device
    // 0xa0 for master, 0xb0 for slave
    port_outb(PORT_ATA_PIO_DRIVE, 0xa0);
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
    uint8_t stat = port_inb(PORT_ATA_PIO_STAT);
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

static void software_reset() {
    port_outb(PORT_ATA_PIO_DEV_CTRL, 4);
    port_outb(PORT_ATA_PIO_DEV_CTRL, 0);
}

// buffer len must be larger or equal to 31
char* ata_pio_get_last_error() {
    uint8_t err = port_inb(PORT_ATA_PIO_ERROR);
    for(int i = 0; i < 8; i++)
        if(err & (1 << i)) return error_msg[i];
    return "it's never reach here";
}

bool ata_pio_LBA28_access(bool read_op, uint32_t lba, unsigned int sector_cnt, uint8_t* buff) {
    if(!LBA28_mode) return false;
    const int slavebit = 0; // idk what is this

    // 0xe0 for master, 0xf0 for slave
    port_outb(PORT_ATA_PIO_DRIVE, 0xe0 | (slavebit << 4) | ((lba >> 24) & 0xf));
    io_wait();
    // send NULL
    port_outb(PORT_ATA_PIO_FEATURE, 0x0);
    // send sector count
    port_outb(PORT_ATA_PIO_SECTOR_COUNT, sector_cnt);
    // send LBA
    port_outb(PORT_ATA_PIO_LBA_LO, (uint8_t)lba);
    port_outb(PORT_ATA_PIO_LBA_MI, (uint8_t)(lba >> 8));
    port_outb(PORT_ATA_PIO_LBA_HI, (uint8_t)(lba >> 16));
    // send command
    if(read_op)
        port_outb(PORT_ATA_PIO_COMM, ATA_PIO_CMD_READ_SECTORS);
    else
        port_outb(PORT_ATA_PIO_COMM, ATA_PIO_CMD_WRITE_SECTORS);

    // poll
    uint8_t stat;
    for(unsigned int j = 0; j < sector_cnt; j++) {
        stat = 0;
        while(stat & 0x8 || stat & 0x1) stat = port_inb(PORT_ATA_PIO_STAT);
        if(stat & 0x1) { // error bit set
            software_reset(); // to clear error bit
            return false;
        }

        uint16_t dat;
        for(int i = 0; i < 256; i++) {
            if(read_op) {
                dat = port_inw(PORT_ATA_PIO_DATA);
                buff[j*512 + i*2] = dat & 0xff;
                buff[j*512 + i*2 + 1] = dat >> 8;
            }
            else {
                dat = (uint16_t)buff[j*512 + i*2] | ((uint16_t)buff[j*512 + i*2 + 1] << 8);
                port_outw(PORT_ATA_PIO_DATA, dat);
            }
        }
    }
    // cache flush after writing
    if(!read_op) port_outb(PORT_ATA_PIO_COMM, ATA_PIO_CMD_CACHE_FLUSH);
    io_wait();

    return true;
}

bool ata_pio_init(uint16_t* buff) {
    uint8_t stat = port_inb(PORT_ATA_PIO_STAT);
    // 0xff is a illegal status return
    // if it is ever returned that means there is no drive
    if(stat == 0xff) return false;

    software_reset();

    bool dev_available = ata_pio_identify();
    if(!dev_available) return false;

    // continue polling until bit 3 (DRQ) or bit 0 (ERR) is set
    stat = 0;
    while(stat & 0x8 || stat & 0x1) stat = port_inb(PORT_ATA_PIO_STAT);
    // if bit 0 (ERR) is set then data is not ready
    if(stat & 0x1) {
        software_reset(); // to clear error bit
        return false;
    }

    for(int i = 0; i < 256; i++) buff[i] = port_inw(PORT_ATA_PIO_DATA);

    // see https://wiki.osdev.org/ATA_PIO_Mode#Interesting_information_returned_by_IDENTIFY
    LBA48_mode = buff[83] & 0x400;
    supported_UDMA = buff[88] & 0xff;
    active_UDMA = buff[88] >> 8;
    cable80 = buff[93] & 0x800;
    total_addressable_sec_LBA28 = buff[60] | (buff[61] << 16); // idk which one is low or high
    total_addressable_sec_LBA48 = (uint64_t)buff[100] |        // also this one
                                 ((uint64_t)buff[101] << 16) | // maybe they are in reversed order
                                 ((uint64_t)buff[102] << 32) |
                                 ((uint64_t)buff[103] << 48);
    LBA28_mode = (total_addressable_sec_LBA28 != 0);

    return true;
}
