#include <filesystem.h>
#include <ata.h>
#include <mem.h>
#include <sys/files.h>

static fs_t ramfs;

static fs_node_t* kern_dev = NULL;
static fs_node_t* kern_stdout = NULL;
static fs_node_t* kern_stdin = NULL;
static fs_node_t* kern_proc = NULL;

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
    if(ramfs_init(&ramfs)) return true;

    if(vfs_find_and_create_node(SYSFILE_PATH_DEV, &ramfs.root_node, &kern_dev, FILE_OPEN_CREATE, FS_NODE_TYPE_DIRECTORY))
        return true;

    if(vfs_find_and_create_node(SYSFILE_PATH_STDIN, kern_dev, &kern_stdin, FILE_OPEN_CREATE, FS_NODE_TYPE_PIPE))
        return true;

    if(vfs_find_and_create_node(SYSFILE_PATH_STDOUT, kern_dev, &kern_stdout, FILE_OPEN_CREATE, FS_NODE_TYPE_PIPE))
        return true;

    if(vfs_find_and_create_node(SYSFILE_PATH_PROC, &ramfs.root_node, &kern_proc, FILE_OPEN_CREATE, FS_NODE_TYPE_DIRECTORY))
        return true;

    // prevent these from being delete from the tree
    kern_dev->refcount = 69420;
    kern_stdin->refcount = 69420;
    kern_stdout->refcount = 69420;
    kern_proc->refcount = 69420;

    return false;
}

fs_node_t* vfs_get_dev() {
    return kern_dev;
}

fs_node_t* vfs_get_stdout() {
    return kern_stdout;
}

fs_node_t* vfs_get_stdin() {
    return kern_stdin;
}

fs_node_t* vfs_get_proc_dir() {
    return kern_proc;
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

fs_t* vfs_get_ramfs() {
    return &ramfs;
}
