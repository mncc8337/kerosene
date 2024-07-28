#include "filesystem.h"

#include "string.h"

static char buffer[512];
static fs_node_t ret_node;

static bool find_node_callback(fs_node_t node) {
    if(node.fs->type == FS_FAT32) {
        if(strcmp_case_insensitive(node.name, buffer)) {
            ret_node = node;
            return false;
        }
    }
    else if(node.fs->type == FS_EXT2) {
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

FS_ERR fs_copy_recursive(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, char* new_name); // declare it first
static fs_node_t copy_current_dir;
static bool cp_node_callback(fs_node_t node) {
    // ignore . and ..
    if(strcmp(node.name, ".") || strcmp(node.name, "..")) return true;
    
    FS_ERR err = fs_copy_recursive(&node, &copy_current_dir, NULL, NULL);
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
fs_node_t fs_find(fs_node_t* parent, const char* path) {
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

// make an empty file in parent node
fs_node_t fs_touch(fs_node_t* parent, char* name) {
    fs_node_t node;
    node.valid = false;

    if(parent->fs->type == FS_FAT32) {
        uint32_t file_cluster = fat32_allocate_clusters(parent->fs, 1);
        if(file_cluster == 0) return node;
        node = fat32_add_entry(parent, name, file_cluster, 0, 0);
    }

    return node;
}

// remove a node
// note that it takes the name not the fs_node_t
// because we still need to find its location in parent directory
// thus sending fs_node_t is overkill
FS_ERR fs_rm(fs_node_t* node, char* name) {
    if(node->fs->type == FS_FAT32)
        return fat32_remove_entry(node, name, true);

    return ERR_FS_UNKNOWN_FS;
}

// remove a directory and its content recursively
FS_ERR fs_rm_recursive(fs_node_t* parent, char* name) {
    // get the node
    fs_node_t delete_node = fs_find(parent, name);

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

// move a node to new parent
// set new_name to NULL to reuse the old name
FS_ERR fs_move(fs_node_t* node, fs_node_t* new_parent, char* new_name) {
    fs_node_t copied;

    if(new_parent->fs->type == FS_FAT32)
        copied = fat32_add_entry(new_parent,
                (new_name == NULL ? node->name : new_name),
                node->start_cluster, node->attr, node->size);
    else return ERR_FS_UNKNOWN_FS;

    if(!copied.valid) return ERR_FS_FAILED;

    FS_ERR err;
    if(node->fs->type == FS_FAT32)
        err = fat32_remove_entry(node->parent_node, node->name, false); // do not delete the node content
    else return ERR_FS_UNKNOWN_FS;

    if(err != ERR_FS_SUCCESS) return err;
    *node = copied;
    return ERR_FS_SUCCESS;
}

FS_ERR fs_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, char* new_name) {
    if(new_parent->fs->type == FS_FAT32) {
        uint32_t start_cluster = fat32_copy_cluster_chain(new_parent->fs, node->start_cluster);
        if(start_cluster == 0) return ERR_FS_FAILED;

        *copied = fat32_add_entry(new_parent,
                (new_name == NULL ? node->name : new_name),
                start_cluster, node->attr, node->size);
    }
    else return ERR_FS_UNKNOWN_FS;

    if(!copied->valid) return ERR_FS_FAILED;
    return ERR_FS_SUCCESS;
}

FS_ERR fs_copy_recursive(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, char* new_name) {
    if(!(node->attr & NODE_DIRECTORY)) return fs_copy(node, new_parent, copied, new_name);

    // the node we need to copy is a directory at this point

    if(new_parent->fs->type == FS_FAT32) {
        // save current dir
        fs_node_t current_dir_bck = copy_current_dir;
        // create a new dir first
        uint32_t start_cluster = fat32_allocate_clusters(new_parent->fs, 1);
        if(start_cluster == 0) return ERR_FS_FAILED;

        *copied = fat32_mkdir(new_parent, (new_name == NULL ? node->name : new_name), start_cluster, node->attr);
        if(!copied->valid) return ERR_FS_FAILED;
        copy_current_dir = *copied;

        // try copy all of its content to the new dir
        if(fat32_read_dir(node, cp_node_callback) != ERR_FS_CALLBACK_STOP)
            return ERR_FS_FAILED;

        // load current dir back up
        copy_current_dir = current_dir_bck;
    }
    else return ERR_FS_UNKNOWN_FS;

    return ERR_FS_SUCCESS;
}

FILE file_open(fs_node_t* node, int mode) {
    FILE file;
    file.valid = false;
    file.node = node;
    file.mode = mode;
    switch(mode) {
        case FILE_WRITE:
            file.valid = true;
            file.position = 0;
            file.current_cluster = node->start_cluster;
            break;
        case FILE_READ:
            file.valid = true;
            file.position = 0;
            file.current_cluster = node->start_cluster;
            break;
        case FILE_APPEND:
            file.valid = true;
            file.position = node->size;
            file.current_cluster = fat32_get_last_cluster_of_chain(node->fs, node->start_cluster);
            break;
    }

    return file;
}

FS_ERR file_write(FILE* file, uint8_t* data, size_t size) {
    if(file->mode == FILE_READ) return ERR_FS_FAILED;

    if(file->node->fs->type == FS_FAT32) {
        fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)file->node->fs->info_table;
        int cluster_size = bootrec->bpb.bytes_per_sector * bootrec->bpb.sectors_per_cluster;

        if(file->position == 0 && file->node->size > (unsigned)cluster_size) {
            // the file may has some infomation before hand
            // so we need to "delete" them first
            FS_ERR err = fat32_cut_cluster_chain(file->node->fs, file->current_cluster);
            if(err != ERR_FS_SUCCESS) return err;
            // reset size since position is 0
            file->node->size = 0;
        }

        FS_ERR err = fat32_write_file(file->node->fs,
            &(file->current_cluster), data, size, file->position % cluster_size);
        if(err != ERR_FS_SUCCESS) return err;

        file->position += size;
        file->node->size += size;
        return ERR_FS_SUCCESS;
    }

    return ERR_FS_UNKNOWN_FS;
}

FS_ERR file_read(FILE* file, uint8_t* buffer, size_t size) {
    if(file->mode != FILE_READ) return ERR_FS_FAILED;

    if(file->position == file->node->size) return ERR_FS_EOF;

    if(file->node->fs->type == FS_FAT32) {
        fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)file->node->fs->info_table;
        int cluster_size = bootrec->bpb.bytes_per_sector * bootrec->bpb.sectors_per_cluster;

        FS_ERR err = fat32_read_file(file->node->fs, &(file->current_cluster), buffer, size, file->position % cluster_size);
        if(err != ERR_FS_SUCCESS) return err;

        file->position += size;
        if(file->position > file->node->size) file->position = file->node->size;
        return ERR_FS_SUCCESS;
    }

    return ERR_FS_UNKNOWN_FS;

}

FS_ERR file_close(FILE* file) {
    if(file->mode == FILE_READ) {
        file->valid = false;
        return ERR_FS_SUCCESS;
    }

    if(file->node->fs->type == FS_FAT32) {
        fat32_update_entry(file->node);
        file->valid = false;
        return ERR_FS_SUCCESS;
    }

    return ERR_FS_UNKNOWN_FS;
}
