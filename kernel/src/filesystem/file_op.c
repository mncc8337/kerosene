#include "filesystem.h"

#include "string.h"

static char name_buffer[FILENAME_LIMIT];
static fs_node_t ret_node;

static bool find_node_callback(fs_node_t node) {
    if(node.fs->type == FS_FAT32) {
        if(strcmp_case_insensitive(node.name, name_buffer)) {
            ret_node = node;
            return false;
        }
    }
    else if(node.fs->type == FS_EXT2) {
        if(strcmp(node.name, name_buffer)) {
            ret_node = node;
            return false;
        }
    }
    return true;
}

FS_ERR fs_rm_recursive(fs_node_t*  parent, fs_node_t delete_node); // declare it first
static bool rm_node_callback(fs_node_t node) {
    // ignore . and ..
    if(strcmp(node.name, ".") || strcmp(node.name, "..")) return true;
    
    FS_ERR err = fs_rm_recursive(node.parent_node, node);
    if(err) return false;

    return true;
}

FS_ERR fs_copy_recursive(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, char* new_name); // declare it first
static fs_node_t copy_current_dir;
static bool cp_node_callback(fs_node_t node) {
    // ignore . and ..
    if(strcmp(node.name, ".") || strcmp(node.name, "..")) return true;
    
    FS_ERR err = fs_copy_recursive(&node, &copy_current_dir, NULL, NULL);
    if(err) return false;

    return true;
}

// go through all files and directories in `parent`. call the callback when found one
FS_ERR fs_list_dir(fs_node_t* parent, bool (*callback)(fs_node_t)) {
    if(parent->fs->type == FS_FAT32)
        return fat32_read_dir(parent, callback);

    return ERR_FS_UNKNOWN_FS;
}

// find a node in parent
fs_node_t fs_find(fs_node_t* parent, const char* nodename) {
    memcpy(name_buffer, nodename, strlen(nodename) + 1);

    FS_NODE_FLAG_UNSET(&ret_node, FS_FLAG_VALID);

    if(parent->fs->type == FS_FAT32) {
        if(fat32_read_dir(parent, find_node_callback) == ERR_FS_CALLBACK_STOP)
            ret_node.flags |= FS_FLAG_VALID;
        else
            FS_NODE_FLAG_UNSET(&ret_node, FS_FLAG_VALID);
    }

    return ret_node;
}

// make a directory in parent node
// return invalid node when failed to find free cluster / fs not defined / directory already exists
fs_node_t fs_mkdir(fs_node_t* parent, char* name) {
    FS_NODE_FLAG_UNSET(&ret_node, FS_FLAG_VALID);

    if(parent->fs->type == FS_FAT32) {
        uint32_t dir_cluster = fat32_allocate_clusters(parent->fs, 1);
        if(dir_cluster == 0) return ret_node;

        ret_node = fat32_mkdir(parent, name, dir_cluster, FAT_ATTR_DIRECTORY);
    }

    return ret_node;
}

// make an empty file in parent node
fs_node_t fs_touch(fs_node_t* parent, char* name) {
    fs_node_t node;
    node.flags = 0;

    if(parent->fs->type == FS_FAT32) {
        uint32_t file_cluster = fat32_allocate_clusters(parent->fs, 1);
        if(file_cluster == 0) return node;
        node = fat32_add_entry(parent, name, file_cluster, 0, 0);
    }

    return node;
}

// remove a node
FS_ERR fs_rm(fs_node_t* node, fs_node_t delete_node) {
    if(node->fs->type == FS_FAT32)
        return fat32_remove_entry(node, delete_node, true);

    return ERR_FS_UNKNOWN_FS;
}

// remove a directory or a file and its content recursively if is a directory
FS_ERR fs_rm_recursive(fs_node_t* parent, fs_node_t delete_node) {
    if(FS_NODE_IS_DIR(delete_node)) {
        // try remove its content recursively
        if(parent->fs->type == FS_FAT32) {
            // this only happended when an error occurs
            if(fat32_read_dir(&delete_node, rm_node_callback) == ERR_FS_CALLBACK_STOP)
                return ERR_FS_FAILED;
        }
        else return ERR_FS_UNKNOWN_FS;

    }

    // now try remove it. it should success
    return fs_rm(parent, delete_node);
}

// move a node to new parent
// set new_name to NULL to reuse the old name
FS_ERR fs_move(fs_node_t* node, fs_node_t* new_parent, char* new_name) {
    fs_node_t copied;

    if(new_parent->fs->type == FS_FAT32) {
        uint8_t attr = 0;
        if(FS_NODE_IS_DIR(*node)) attr |= FAT_ATTR_DIRECTORY;
        if(FS_NODE_IS_HIDDEN(*node)) attr |= FAT_ATTR_HIDDEN;
        copied = fat32_add_entry(
            new_parent,
            new_name == NULL ? node->name : new_name,
            node->fat_cluster.start_cluster, attr, node->size
        );
    }
    else return ERR_FS_UNKNOWN_FS;

    if(!FS_NODE_IS_VALID(copied)) return ERR_FS_FAILED;

    FS_ERR err;
    if(node->fs->type == FS_FAT32)
        err = fat32_remove_entry(node->parent_node, *node, false); // do not delete the node content
    else return ERR_FS_UNKNOWN_FS;

    if(err) return err;
    *node = copied;
    return ERR_FS_SUCCESS;
}

FS_ERR fs_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, char* new_name) {
    if(new_parent->fs->type == FS_FAT32) {
        uint32_t start_cluster = fat32_copy_cluster_chain(new_parent->fs, node->fat_cluster.start_cluster);
        if(start_cluster == 0) return ERR_FS_FAILED;

        uint8_t attr = 0;
        if(FS_NODE_IS_DIR(*node)) attr |= FAT_ATTR_DIRECTORY;
        if(FS_NODE_IS_HIDDEN(*node)) attr |= FAT_ATTR_HIDDEN;
        *copied = fat32_add_entry(
            new_parent,
            new_name == NULL ? node->name : new_name,
            start_cluster, attr, node->size
        );
    }
    else return ERR_FS_UNKNOWN_FS;

    if(!FS_NODE_IS_VALID(*copied)) return ERR_FS_FAILED;
    return ERR_FS_SUCCESS;
}

FS_ERR fs_copy_recursive(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, char* new_name) {
    if(!FS_NODE_IS_DIR(*node)) return fs_copy(node, new_parent, copied, new_name);

    // the node we need to copy is a directory at this point

    if(new_parent->fs->type == FS_FAT32) {
        // save current dir
        fs_node_t current_dir_bck = copy_current_dir;
        // create a new dir first
        uint32_t start_cluster = fat32_allocate_clusters(new_parent->fs, 1);
        if(start_cluster == 0) return ERR_FS_FAILED;

        uint8_t attr = FAT_ATTR_DIRECTORY;
        if(FS_NODE_IS_HIDDEN(*node)) attr |= FAT_ATTR_HIDDEN;
        *copied = fat32_mkdir(
            new_parent,
            new_name == NULL ? node->name : new_name,
            start_cluster, attr
        );
        if(!FS_NODE_IS_VALID(*copied)) return ERR_FS_FAILED;
        copy_current_dir = *copied;

        // try copy all of its content to the new dir
        if(fat32_read_dir(node, cp_node_callback) == ERR_FS_CALLBACK_STOP)
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
            file.fat32_current_cluster = node->fat_cluster.start_cluster;
            break;
        case FILE_READ:
            file.valid = true;
            file.position = 0;
            file.fat32_current_cluster = node->fat_cluster.start_cluster;
            break;
        case FILE_APPEND:
            file.valid = true;
            file.position = node->size;
            file.fat32_current_cluster = fat32_get_last_cluster_of_chain(node->fs, node->fat_cluster.start_cluster);
            break;
    }

    if(file.valid)
        node->accessed_timestamp = time(NULL);

    return file;
}

FS_ERR file_write(FILE* file, uint8_t* data, size_t size) {
    if(file->mode == FILE_READ) return ERR_FS_FAILED;

    if(file->node->fs->type == FS_FAT32) {
        fat32_bootrecord_t* bootrec = &(file->node->fs->fat32_info.bootrec);
        int cluster_size = bootrec->bpb.bytes_per_sector * bootrec->bpb.sectors_per_cluster;

        if(file->position == 0 && file->node->size > (unsigned)cluster_size) {
            // the file may has some infomation before hand
            // so we need to "delete" them first
            FS_ERR err = fat32_cut_cluster_chain(file->node->fs, file->fat32_current_cluster);
            if(err) return err;
            // reset size since position is 0
            file->node->size = 0;
        }

        int offset = file->position % cluster_size;
        if(offset == 0 && file->position > 0) {
            // send offset = 512 so we will know to change to the next cluster
            offset = 512;
        }
        FS_ERR err = fat32_write_file(
            file->node->fs,
            &(file->fat32_current_cluster), data, size, offset
        );
        if(err) return err;
    }
    else return ERR_FS_UNKNOWN_FS;

    file->position += size;
    file->node->size += size;
    file->node->modified_timestamp = time(NULL);
    return ERR_FS_SUCCESS;
}

FS_ERR file_read(FILE* file, uint8_t* buffer, size_t size) {
    if(file->mode != FILE_READ) return ERR_FS_FAILED;

    if(file->position == file->node->size) return ERR_FS_EOF;

    FS_ERR err;
    if(file->node->fs->type == FS_FAT32) {
        fat32_bootrecord_t* bootrec = &(file->node->fs->fat32_info.bootrec);
        int cluster_size = bootrec->bpb.bytes_per_sector * bootrec->bpb.sectors_per_cluster;

        int offset = file->position % cluster_size;
        if(offset == 0 && file->position > 0) {
            // send offset = 512 so we will know to change to the next cluster
            offset = 512;
        }
        err = fat32_read_file(
            file->node->fs,
            &(file->fat32_current_cluster), buffer, size, offset
        );
    }
    else return ERR_FS_UNKNOWN_FS;

    if(err) return err;
    file->position += size;
    if(file->position > file->node->size) file->position = file->node->size;
    return ERR_FS_SUCCESS;

}

FS_ERR file_close(FILE* file) {
    if(file->node->fs->type == FS_FAT32)
        fat32_update_entry(file->node);
    else return ERR_FS_UNKNOWN_FS;

    file->valid = false;
    return ERR_FS_SUCCESS;
}
