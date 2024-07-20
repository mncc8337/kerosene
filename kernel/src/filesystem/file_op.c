#include "filesystem.h"

#include "string.h"
#include "debug.h"

static char buffer[512];
static FS_NODE ret_node;

static uint32_t current_cluster;

static bool find_file(FS_NODE node) {
    if(node.attr == 0x10) return true;
    if(strcmp(node.name, buffer)) {
        ret_node = node;
        return false;
    }
    return true;
}

bool file_set_current_dir(FS_NODE node) {
    if(node.attr != 0x10) return false;
    current_cluster = node.start_cluster;
    return true;
}

void file_set_current_cluster(uint32_t cluster) {
    current_cluster = cluster;
}

bool file_list_dir(FILESYSTEM* fs, bool (*callback)(FS_NODE)) {
    if(fs->type == FS_FAT32) {
        fat32_read_dir((FAT32_BOOT_RECORD_t*)(fs->info_table),
                    current_cluster, fs->partition.LBA_start, callback);
        return true;
    }

    return false;
}

FS_NODE file_open(FILESYSTEM* fs, const char* filename) {
    memcpy(buffer, filename, strlen(filename) + 1);

    if(fs->type == FS_FAT32) {
        if(!fat32_read_dir((FAT32_BOOT_RECORD_t*)(fs->info_table),
                    current_cluster, fs->partition.LBA_start, find_file))
            ret_node.valid = true;
        else ret_node.valid = false;
        return ret_node;
    }

    // default return
    ret_node.valid = false;
    return ret_node;
}

bool file_read(FILESYSTEM* fs, FS_NODE node, uint8_t* buffer) {
    if(fs->type == FS_FAT32) {
        fat32_read_file((FAT32_BOOT_RECORD_t*)(fs->info_table),
                node.start_cluster, fs->partition.LBA_start, buffer);
        return true;
    }

    return false;
}
