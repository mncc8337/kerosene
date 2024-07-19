#include "filesystem.h"
#include "ata.h"

#include "string.h"
#include "debug.h"

static bool is_field_fs_type(uint8_t* buff, int cnt) {
    for(int i = 0; i < cnt; i++) {
        if(buff[i] != 0x20 // space
                && (buff[i] < 0x41 || buff[i] > 0x5a) // uppercase
                && (buff[i] < 0x61 || buff[i] > 0x7a) // lowercase
                && (buff[i] < 0x30 || buff[i] > 0x39)) // number
            return false;
    }

    return true;
}

FS_TYPE detect_fs(partition_entry_t part) {
    uint8_t sect[512];

    // read the first sector
    ata_pio_LBA28_access(true, part.LBA_start, 1, sect);

    // i think using goto label is much cleaner than pure if else

// FAT32:
    FAT32_BOOT_RECORD_t* fat32_bootrec = (FAT32_BOOT_RECORD_t*)sect;

    // check BPB 7.0 signature
    if(fat32_bootrec->ebpb.signature == 0x28 || fat32_bootrec->ebpb.signature == 0x29) {
        // check the filesystem type field
        if(!is_field_fs_type(fat32_bootrec->ebpb.system_identifier, 8))
            goto FAT_12_16;
    }
    else goto FAT_12_16;

    return FS_FAT32;

FAT_12_16:
    FAT_BOOT_RECORD_t* fat_bootrec = (FAT_BOOT_RECORD_t*)sect;
    // check BPB 4.0 signature
    if(fat_bootrec->ebpb.signature == 0x28 || fat_bootrec->ebpb.signature == 0x29) {
        // check the filesystem type field
        if(!is_field_fs_type(fat_bootrec->ebpb.system_identifier, 8))
            goto EXT_234;
    }
    else goto EXT_234;

    return FS_FAT_12_16;

EXT_234:
    // TODO: implement this

    return FS_EMPTY;
}
