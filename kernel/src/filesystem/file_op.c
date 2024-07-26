#include "filesystem.h"

#include "string.h"

static char buffer[512];
static fs_node_t ret_node;

static bool find_node_callback(fs_node_t node) {
    if(node.fs->type == FS_FAT32 || node.fs->type == FS_FAT_12_16) {
        if(strcmp_case_insensitive(node.name, buffer)) {
            ret_node = node;
            return false;
        }
    }
    else if(node.fs->type == FS_EXT2 || node.fs->type == FS_EXT3 || node.fs->type == FS_EXT4) {
        if(strcmp(node.name, buffer)) {
            ret_node = node;
            return false;
        }
    }
    return true;
}

static fs_node_t find_node(fs_node_t* parent, const char* nodename) {
    memcpy(buffer, nodename, strlen(nodename) + 1);

    if(parent->fs->type == FS_FAT32) {
        if(fat32_read_dir(parent, find_node_callback) == ERR_FS_CALLBACK_STOP) ret_node.valid = true;
        else ret_node.valid = false;
        return ret_node;
    }

    // default return
    ret_node.valid = false;
    return ret_node;
}

// go through all files and directories in `parent`. call the callback when found one
bool fs_list_dir(fs_node_t* parent, bool (*callback)(fs_node_t)) {
    if(parent->fs->type == FS_FAT32) {
        fat32_read_dir(parent, callback); return true;
    }

    return false;
}

// find a node using path
fs_node_t fs_find_node(fs_node_t* parent, const char* path) {
    fs_node_t current_node = *parent;

    // make a copy
    int path_len = strlen(path);
    char path_cpy[path_len+1];
    memcpy(path_cpy, path, path_len+1);

    char* nodename = strtok(path_cpy, "/");

    while(nodename != NULL) {
        current_node = find_node(&current_node, nodename);
        if(!current_node.valid) break;

        nodename = strtok(NULL, "/");
    }

    return current_node;
}

// read a node content to the buffer
bool fs_read_node(fs_node_t* node, uint8_t* buffer) {
    if(node->fs->type == FS_FAT32) {
        fat32_read_file(node, buffer); return true;
    }

    return false;
}

// make a directory in parent node
// return invalid node when failed to find free cluster / fs not defined / directory already exists
fs_node_t fs_mkdir(fs_node_t* parent, char* name) {
    ret_node.valid = false;

    if(parent->fs->type == FS_FAT32) {
        uint32_t dir_cluster = fat32_allocate_clusters(parent->fs, 1);
        if(dir_cluster == 0) return ret_node;

        ret_node = fat32_mkdir(parent, name, dir_cluster, NODE_DIRECTORY);
        return ret_node;
    }

    return ret_node;
}
