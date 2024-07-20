#include "filesystem.h"
#include "ata.h"

#include "string.h"
#include "debug.h"

static FILESYSTEM FS[26];

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

static bool fat32_check(uint8_t* sect) {
    FAT32_BOOT_RECORD_t* fat32_bootrec = (FAT32_BOOT_RECORD_t*)sect;

    // check BPB 7.0 signature
    if(fat32_bootrec->ebpb.signature != 0x28 && fat32_bootrec->ebpb.signature != 0x29)
        return false;

    // check the filesystem type field
    if(!is_field_fs_type(fat32_bootrec->ebpb.system_identifier, 8))
        return false;

    return true;
}

static bool fat_12_16_check(uint8_t* sect) {
    FAT_BOOT_RECORD_t* fat_bootrec = (FAT_BOOT_RECORD_t*)sect;

    // check BPB 4.0 signature
    if(fat_bootrec->ebpb.signature != 0x28 && fat_bootrec->ebpb.signature != 0x29)
        return false;

    // check the filesystem type field
    if(!is_field_fs_type(fat_bootrec->ebpb.system_identifier, 8))
        return false;

    return true;
}

// TODO: implement these
// static bool ext2_check(uint8_t* sect) {
//     return false;
// }
// static bool ext3_check(uint8_t* sect) {
//     return false;
// }
// static bool ext4_check(uint8_t* sect) {
//     return false;
// }

FS_TYPE fs_detect(partition_entry_t part) {
    uint8_t sect[512];
    ata_pio_LBA28_access(true, part.LBA_start, 1, sect);

    if(fat32_check(sect)) return FS_FAT32;
    if(fat_12_16_check(sect)) return FS_FAT_12_16;
    // if(ext2_check(sect)) return FS_EXT2;
    // if(ext3_check(sect)) return FS_EXT3;
    // if(ext4_check(sect)) return FS_EXT4;

    return FS_EMPTY;
}

void fs_add(FILESYSTEM fs, int id) {
    FS[id] = fs;
}

FILESYSTEM* fs_get(int id) {
    return &(FS[id]);
}
