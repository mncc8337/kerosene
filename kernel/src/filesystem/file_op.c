#include "filesystem.h"

#include "string.h"
#include "debug.h"

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

FS_ERR fs_rm_recursive(fs_node_t*  parent, char* name); // declare it first
static bool rm_node_callback(fs_node_t node) {
    // ignore . and ..
    if(strcmp(node.name, ".") || strcmp(node.name, "..")) return true;
    
    FS_ERR err = fs_rm_recursive(node.parent_node, node.name);
    if(err != ERR_FS_SUCCESS) return false;

    return true;
}

// go through all files and directories in `parent`. call the callback when found one
FS_ERR fs_list_dir(fs_node_t* parent, bool (*callback)(fs_node_t)) {
    if(parent->fs->type == FS_FAT32)
        return fat32_read_dir(parent, callback);

    return ERR_FS_UNKNOWN_FS;
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
FS_ERR fs_read_node(fs_node_t* node, uint8_t* buffer) {
    if(node->fs->type == FS_FAT32)
        return fat32_read_file(node, buffer);

    return ERR_FS_UNKNOWN_FS;
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

// remove a node
// note that it takes the name not the fs_node_t
// because we still need to find its location in parent directory
// thus sending fs_node_t is overkill
FS_ERR fs_rm(fs_node_t* node, char* name) {
    if(node->fs->type == FS_FAT32)
        return fat32_remove_entry(node, name);

    return ERR_FS_UNKNOWN_FS;
}

// remove a directory and its content recursively
FS_ERR fs_rm_recursive(fs_node_t*  parent, char* name) {
    // obviously doing this in low level is way faster
    // but it didn't work for some reason

    // get the node
    fs_node_t delete_node = fs_find_node(parent, name);

    if(delete_node.attr & NODE_DIRECTORY) {
        FS_ERR err;

        // try remove its content recursively
        if(parent->fs->type == FS_FAT32)
            err = fat32_read_dir(&delete_node, rm_node_callback);
        else return ERR_FS_UNKNOWN_FS;

        if(err == ERR_FS_CALLBACK_STOP) {
            // this only happended when an error occurs
            return ERR_FS_FAILED;
        }
    }

    // now try remove it. it should success
    return fs_rm(parent, name);
}
