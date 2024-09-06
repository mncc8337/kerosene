#include "filesystem.h"
#include "ata.h"

#include "debug.h"

static mbr_t MBR;

bool mbr_load() {
    ATA_PIO_ERR err = ata_pio_LBA28_access(true, 0, 1, (uint8_t*)(&MBR));
    if(err) {
        print_debug(LT_ER, "error while reading bootsector: %s\n", ata_pio_get_error());
        return true;
    }

    // verify if it is bootsector
    if(MBR.boot_signature != 0xaa55) {
        print_debug(LT_ER, "sector not contain boot signature\n");
        return true;
    }

    return false;
}

partition_entry_t mbr_get_partition_entry(unsigned int id) {
    id = id % 4; // do not pass 4
    return MBR.partition_entry[id];
}
