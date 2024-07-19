#include "filesystem.h"
#include "ata.h"

#include "string.h"
#include "debug.h"

static bool is_field_fs_type(uint8_t* buff, int from, int to) {
    for(int i = from; i <= to; i++)
        if(buff[i] != 0x20 && (buff[i] < 0x41 || buff[i] > 0x5a) && (buff[i] < 0x61 || buff[i] > 0x7a))
            return false;

    return true;
}

FS_TYPE detect_fs(partition_entry_t part) {
    uint8_t sect[512];

    // read the first sector
    ata_pio_LBA28_access(true, part.LBA_start, 1, sect);

    // i think using goto label is much cleaner than pure if else

// FAT32:
    // check BPB 7.0 signature
    if(sect[0x42] == 0x28 || sect[0x42] == 0x29) {
        // check the filesystem type field
        if(!is_field_fs_type(sect, 0x52, 0x69))
            goto FAT_12_16;
    }
    else goto FAT_12_16;

    return FS_FAT32;

FAT_12_16:
    // check BPB 4.0 signature
    if(sect[0x26] == 0x28 || sect[0x26] == 0x29) {
        // check fs type field
        if(!is_field_fs_type(sect, 0x36, 0x3d))
            goto EXT_234;
    }
    else goto EXT_234;

    return FS_FAT_12_16;

EXT_234:
    // TODO: implement this

    return FS_EMPTY;
}
