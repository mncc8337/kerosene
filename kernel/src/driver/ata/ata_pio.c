#include "ata.h"
#include "system.h"

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

static void wait_400ns() {
    port_inb(PORT_ATA_PIO_ALT_STAT);
    port_inb(PORT_ATA_PIO_ALT_STAT);
    port_inb(PORT_ATA_PIO_ALT_STAT);
    port_inb(PORT_ATA_PIO_ALT_STAT);
}

static uint8_t wait_until_not_busy() {
    uint8_t stat;
    while((stat = port_inb(PORT_ATA_PIO_STAT)) & ATA_PIO_STAT_BSY);
    return stat;
}

static ATA_PIO_ERR wait_until_data_ready() {
    uint8_t stat;
    while((stat = port_inb(PORT_ATA_PIO_STAT)) & (ATA_PIO_STAT_BSY | ATA_PIO_STAT_DRQ));
    if(stat & ATA_PIO_STAT_ERR) return ERR_ATA_PIO_ERR_BIT_SET;
    if(stat & ATA_PIO_STAT_DF) return ERR_ATA_PIO_DRIVE_FAULT;
    return ERR_ATA_PIO_SUCCESS;
}

static ATA_PIO_ERR wait_ata(bool return_error) {
    wait_400ns();

    uint8_t stat = wait_until_not_busy();

    if(return_error) {
        stat = port_inb(PORT_ATA_PIO_STAT);
        if(stat & ATA_PIO_STAT_ERR) return ERR_ATA_PIO_ERR_BIT_SET;
        if(stat & ATA_PIO_STAT_DF) return ERR_ATA_PIO_DRIVE_FAULT;
    }
    return ERR_ATA_PIO_SUCCESS;
}

static void software_reset() {
    port_outb(PORT_ATA_PIO_DEV_CTRL, 4);
    wait_400ns();
    port_outb(PORT_ATA_PIO_DEV_CTRL, 0);
}

static bool ata_pio_identify() {
    // select target device
    // 0xa0 for master, 0xb0 for slave
    port_outb(PORT_ATA_PIO_DRIVE, 0xa0);
    wait_400ns();
    // set some stuffs
    port_outb(PORT_ATA_PIO_SECTOR_COUNT, 0);
    port_outb(PORT_ATA_PIO_LBA_LO, 0);
    port_outb(PORT_ATA_PIO_LBA_MI, 0);
    port_outb(PORT_ATA_PIO_LBA_HI, 0);
    // send IDENTIFY command
    port_outb(PORT_ATA_PIO_COMM, ATA_PIO_CMD_IDENTIFY);
    wait_400ns();

    // read status port
    uint8_t stat = port_inb(PORT_ATA_PIO_STAT);
    if(stat == 0) // drive does not exists
        return ERR_ATA_PIO_NO_DEV;

    wait_until_not_busy();

    // check if LBA low and LBA high are non-zero
    // if so they are not ATA device
    if(port_inb(PORT_ATA_PIO_LBA_LO) && port_inb(PORT_ATA_PIO_LBA_HI))
        return ERR_ATA_PIO_NO_DEV;

    port_outb(PORT_ATA_PIO_DEV_CTRL, 0x2);

    return ERR_ATA_PIO_SUCCESS;
}

char* ata_pio_get_error() {
    uint8_t err = port_inb(PORT_ATA_PIO_ERROR);
    int i;
    for(i = 0; i < 8; i++)
        if(err & (1 << i)) break;
    software_reset();
    return error_msg[i];
}

ATA_PIO_ERR ata_pio_LBA28_access(bool read_op, uint32_t lba, unsigned int sector_cnt, uint8_t* buff) {
    if(!LBA28_mode) return ERR_ATA_PIO_METHOD_NOT_AVAILABLE;
    if(sector_cnt == 0) return ERR_ATA_PIO_INVALID_PARAMS;
    const int slavebit = 0; // idk what is this

    port_outb(PORT_ATA_PIO_DEV_CTRL, 0x2);
    wait_until_not_busy();

    // 0xe0 for master, 0xf0 for slave
    port_outb(PORT_ATA_PIO_DRIVE, 0xe0 | (slavebit << 4) | ((lba >> 24) & 0xf));
    // send NULL
    port_outb(PORT_ATA_PIO_FEATURE, 0x0);
    // send sector count
    port_outb(PORT_ATA_PIO_SECTOR_COUNT, sector_cnt);
    // send LBA
    port_outb(PORT_ATA_PIO_LBA_LO, lba & 0xff);
    port_outb(PORT_ATA_PIO_LBA_MI, (lba >> 8) & 0xff);
    port_outb(PORT_ATA_PIO_LBA_HI, (lba >> 16) & 0xff);
    // send command
    if(read_op)
        port_outb(PORT_ATA_PIO_COMM, ATA_PIO_CMD_READ_SECTORS);
    else
        port_outb(PORT_ATA_PIO_COMM, ATA_PIO_CMD_WRITE_SECTORS);

    ATA_PIO_ERR err = wait_ata(1);
    if(err != ERR_ATA_PIO_SUCCESS) return err;

    uint16_t dat;
    for(unsigned int j = 0; j < sector_cnt; j++) {
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
        err = wait_until_data_ready();
        if(err != ERR_ATA_PIO_SUCCESS) return err;
    }
    if(!read_op) port_outb(PORT_ATA_PIO_COMM, ATA_PIO_CMD_CACHE_FLUSH);

    wait_ata(0);
    return ERR_ATA_PIO_SUCCESS;
}

// TODO: implement LBA48 access (do i even need it lol)

ATA_PIO_ERR ata_pio_init(uint16_t* buff) {
    uint8_t stat = port_inb(PORT_ATA_PIO_STAT);
    // 0xff is a illegal status return
    // if it is ever returned that means there is no drive
    if(stat == 0xff) return ERR_ATA_PIO_NO_DEV;

    software_reset();

    bool dev_available = ata_pio_identify();
    if(!dev_available) return ERR_ATA_PIO_NO_DEV;

    ATA_PIO_ERR err = wait_ata(1);
    if(err != ERR_ATA_PIO_SUCCESS) return err;

    for(int i = 0; i < 256; i++) buff[i] = port_inw(PORT_ATA_PIO_DATA);

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

    return ERR_ATA_PIO_SUCCESS;
}
