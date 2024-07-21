#include "filesystem.h"

#include "string.h"

static char buffer[512];
static FS_NODE ret_node;

static FS_NODE current_node;

static bool find_node_callback(FS_NODE node) {
    if(strcmp(node.name, buffer)) {
        ret_node = node;
        return false;
    }
    return true;
}

static FS_NODE find_node(FILESYSTEM* fs, uint32_t start_cluster, const char* nodename) {
    memcpy(buffer, nodename, strlen(nodename) + 1);

    if(fs->type == FS_FAT32) {
        if(!fat32_read_dir((FAT32_BOOT_RECORD_t*)(fs->info_table),
                    start_cluster, fs->partition.LBA_start, find_node_callback))
            ret_node.valid = true;
        else ret_node.valid = false;
        return ret_node;
    }

    // default return
    ret_node.valid = false;
    return ret_node;
}

bool fs_set_current_node(FS_NODE node) {
    // only accept directory
    if(!(node.attr & 0x10)) return false;

    current_node = node;
    return true;
}
FS_NODE fs_get_current_node() {
    return current_node;
}

// go through all files and directories. call the callback when found one
bool fs_list_dir(FILESYSTEM* fs, bool (*callback)(FS_NODE)) {
    if(fs->type == FS_FAT32) {
        fat32_read_dir((FAT32_BOOT_RECORD_t*)(fs->info_table),
                    current_node.start_cluster, fs->partition.LBA_start, callback);
        return true;
    }

    return false;
}

// find a node using relative path
FS_NODE fs_find_node(FILESYSTEM* fs, const char* path) {
    FS_NODE __current_node = current_node;

    // make a copy
    int path_len = strlen(path);
    char path_cpy[path_len+1];
    memcpy(path_cpy, path, path_len+1);

    char* nodename = strtok(path_cpy, "/");

    while(nodename != NULL) {
        __current_node = find_node(fs, __current_node.start_cluster, nodename);
        if(!__current_node.valid) break;

        nodename = strtok(NULL, "/");
    }

    return __current_node;
}

// read a node content to the buffer
bool fs_read_node(FILESYSTEM* fs, FS_NODE node, uint8_t* buffer) {
    if(fs->type == FS_FAT32) {
        fat32_read_file((FAT32_BOOT_RECORD_t*)(fs->info_table),
                node.start_cluster, fs->partition.LBA_start, buffer);
        return true;
    }

    return false;
}
