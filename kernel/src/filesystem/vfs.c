#include "filesystem.h"
#include "ata.h"
#include "mem.h"

static fs_t* FS;
static file_description_t KERNEL_FDT[MAX_FILE];
static unsigned KERNEL_FILE_COUNT = 0;

static bool fat32_check(uint8_t* sect) {
    fat32_bootrecord_t* fat32_bootrec = (fat32_bootrecord_t*)sect;

    // check BPB 7.0 signature
    if(fat32_bootrec->ebpb.signature != 0x28 && fat32_bootrec->ebpb.signature != 0x29)
        return false;

    // check the filesystem type field
    uint8_t* buff = fat32_bootrec->ebpb.system_identifier;
    for(int i = 0; i < 8; i++) {
        if(buff[i] != 0x20 // space
                && (buff[i] < 0x41 || buff[i] > 0x5a) // uppercase
                && (buff[i] < 0x61 || buff[i] > 0x7a) // lowercase
                && (buff[i] < 0x30 || buff[i] > 0x39)) // number
            return false;
    }

    return true;
}

// TODO: implement EXT* check
// static bool ext2_check(uint8_t* sect) {
//     return false;
// }

bool vfs_init() {
    FS = (fs_t*)kmalloc(sizeof(fs_t) * MAX_FS);
    if(!FS) return true;

    for(unsigned i = 0; i < MAX_FS; i++)
        FS[i].type = FS_EMPTY;

    if(ramfs_init(&FS[RAMFS_DISK])) {
        kfree(FS);
        return true;
    }

    return false;
}

fs_type_t vfs_detectfs(partition_entry_t* part) {
    uint8_t sect[512];
    ata_pio_LBA28_access(true, part->LBA_start, 1, sect);

    if(fat32_check(sect)) return FS_FAT32;
    // if(ext2_check(sect)) return FS_EXT2;
    // if(ext3_check(sect)) return FS_EXT3;
    // if(ext4_check(sect)) return FS_EXT4;

    return FS_EMPTY;
}

fs_t* vfs_getfs(int id) {
    if(id >= MAX_FS) return 0;
    return FS + id;
}

bool vfs_is_fs_available(int id) {
    if(id >= MAX_FS) return false;
    return FS[id].type != FS_EMPTY;
}

file_description_t* vfs_get_kernel_file_descriptor_table() {
    return KERNEL_FDT;
}

unsigned* vfs_get_kernel_file_count() {
    return &KERNEL_FILE_COUNT;
}
