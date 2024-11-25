#include "filesystem.h"

#include "string.h"

// static fs_node_t copy_current_dir;
// static bool cp_node_callback(fs_node_t node) {
//     // ignore . and ..
//     if(strcmp(node.name, ".") || strcmp(node.name, "..")) return true;
//
//     FS_ERR err = fs_copy_recursive(&node, &copy_current_dir, NULL, NULL);
//     if(err) return false;
//
//     return true;
// }

FS_ERR fs_setup_directory_iterator(directory_iterator_t* diriter, fs_node_t* node) {
    switch(node->fs->type) {
        case FS_FAT32:
            return fat32_setup_directory_iterator(diriter, node);
        case FS_RAMFS:
            return ramfs_setup_directory_iterator(diriter, node);
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
}

FS_ERR fs_read_dir( directory_iterator_t* diriter, fs_node_t* ret_node) {
    switch(diriter->node->fs->type) {
        case FS_FAT32:
            return fat32_read_dir(diriter, ret_node);
        case FS_RAMFS:
            return ramfs_read_dir(diriter, ret_node);
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
}

FS_ERR fs_find(fs_node_t* parent, const char* nodename, fs_node_t* ret_node) {
    FS_ERR last_err;
    ret_node->flags = 0;

    directory_iterator_t diriter;
    FS_ERR diriter_err = fs_setup_directory_iterator(&diriter, parent);
    if(diriter_err) return diriter_err;

    bool (*cmpcmd)(const char*, const char*);
    if(parent->fs->type == FS_FAT32) cmpcmd = strcmp_case_insensitive;
    else cmpcmd = strcmp;

    while(!(last_err = fs_read_dir(&diriter, ret_node))) {
        if(cmpcmd(nodename, ret_node->name))
            return ERR_FS_SUCCESS;
    }

    if(last_err != ERR_FS_EOF)
        return last_err;

    return ERR_FS_NOT_FOUND;
}

// make a directory in parent node
// return invalid node when failed to find free cluster / fs not defined / directory already exists
FS_ERR fs_mkdir(fs_node_t* parent, char* name, fs_node_t* new_node) {
    new_node->flags = 0;

    switch(parent->fs->type) {
        case FS_FAT32:
            return fat32_mkdir(parent, name, 0, new_node);
        case FS_RAMFS:
            return ramfs_mkdir(parent, name, 0, new_node);
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
}

FS_ERR fs_touch(fs_node_t* parent, char* name, fs_node_t* new_node) {
    new_node->flags = 0;

    uint32_t file_cluster;
    ramfs_datanode_t* datanode_chain;

    switch(parent->fs->type) {
        case FS_FAT32:
            file_cluster = fat32_allocate_clusters(parent->fs, 1, false);
            if(!file_cluster) return ERR_FS_NOT_ENOUGH_SPACE;
            return fat32_add_entry(parent, name, file_cluster, 0, 0, new_node);
        case FS_RAMFS:
            datanode_chain = ramfs_allocate_datanodes(1, false);
            if(!datanode_chain) return ERR_FS_NOT_ENOUGH_SPACE;
            return ramfs_add_entry(parent, name, datanode_chain, 0, 0, new_node);
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
}

FS_ERR fs_rm(fs_node_t* node, fs_node_t* delete_node) {
    switch(node->fs->type) {
        case FS_FAT32:
            return fat32_remove_entry(node, delete_node, true);
        case FS_RAMFS:
            return ramfs_remove_entry(node, delete_node, true);
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
}

FS_ERR fs_rm_recursive(fs_node_t* parent, fs_node_t* delete_node) {
    if(FS_NODE_IS_DIR(*delete_node)) {
        // try to remove all of its contents
        directory_iterator_t diriter;
        fs_setup_directory_iterator(&diriter, delete_node);

        fs_node_t child_node;
        FS_ERR last_err;
        while(!(last_err = fs_read_dir(&diriter, &child_node))) {
            if(strcmp(child_node.name, ".") || strcmp(child_node.name, "..")) continue;
            fs_rm_recursive(delete_node, &child_node);
        }

        if(last_err != ERR_FS_EOF) return last_err;
    }

    // now try remove it. it should success
    return fs_rm(parent, delete_node);
}

FS_ERR fs_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, char* new_name) {
    // if(new_parent->fs->type == FS_FAT32) {
    //     uint32_t start_cluster = fat32_copy_cluster_chain(new_parent->fs, node->fat_cluster.start_cluster);
    //     if(start_cluster == 0) return ERR_FS_FAILED;
    //
    //     *copied = fat32_add_entry(
    //         new_parent,
    //         new_name == NULL ? node->name : new_name,
    //         start_cluster, fat32_to_fat_attr(node->flags), node->size
    //     );
    // }
    // else return ERR_FS_NOT_SUPPORTED;
    //
    // if(!FS_NODE_IS_VALID(*copied)) return ERR_FS_FAILED;
    return ERR_FS_SUCCESS;
}

FS_ERR fs_copy_recursive(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, char* new_name) {
    // if(!FS_NODE_IS_DIR(*node)) return fs_copy(node, new_parent, copied, new_name);
    //
    // // the node we need to copy is a directory at this point
    //
    // if(new_parent->fs->type == FS_FAT32) {
    //     // save current dir
    //     fs_node_t current_dir_bck = copy_current_dir;
    //
    //     // create a new dir first
    //     uint8_t attr = FAT_ATTR_DIRECTORY;
    //     if(FS_NODE_IS_HIDDEN(*node)) attr |= FAT_ATTR_HIDDEN;
    //     *copied = fat32_mkdir(
    //         new_parent,
    //         new_name == NULL ? node->name : new_name,
    //         attr
    //     );
    //     if(!FS_NODE_IS_VALID(*copied)) return ERR_FS_FAILED;
    //     copy_current_dir = *copied;
    //
    //     // try copy all of its content to the new dir
    //     if(fat32_read_dir(node, cp_node_callback) == ERR_FS_CALLBACK_STOP)
    //         return ERR_FS_FAILED;
    //
    //     // load current dir back up
    //     copy_current_dir = current_dir_bck;
    // }
    // else return ERR_FS_NOT_SUPPORTED;

    return ERR_FS_SUCCESS;
}

// move a node to new parent
// set new_name to NULL to reuse the old name
FS_ERR fs_move(fs_node_t* node, fs_node_t* new_parent, char* new_name) {
    if(node->fs->type == new_parent->fs->type) {
        switch(node->fs->type) {
            case FS_FAT32:
                return fat32_move(node, new_parent, new_name);
            case FS_RAMFS:
                return ramfs_move(node, new_parent, new_name);
            default:
                return ERR_FS_NOT_SUPPORTED;
        }
    }
    else {
        return ERR_FS_NOT_SUPPORTED;
        // TODO:
        // copy node to new_parent
        // then delete the old node
    }
}

#include "stdio.h"
FS_ERR file_open(FILE* file, fs_node_t* node, int mode) {
    file->node = node;
    file->mode = mode;
    if(mode == FILE_WRITE || mode == FILE_READ)
        file->position = 0;
    else if(mode == FILE_APPEND)
        file->position = node->size;
    else return ERR_FS_NOT_SUPPORTED;

    switch(node->fs->type) {
        case FS_FAT32:
            if(mode == FILE_WRITE || mode == FILE_READ) {
                file->fat32_current_cluster = node->fat_cluster.start_cluster;
            }
            else if(mode == FILE_APPEND) {
                file->fat32_current_cluster = fat32_get_last_cluster_of_chain(
                    node->fs,
                    node->fat_cluster.start_cluster
                );
            }
            break;
        case FS_RAMFS:
            if(mode == FILE_WRITE || mode == FILE_READ) {
                file->ramfs_current_datanode = (uint32_t)((ramfs_node_t*)node->ramfs_node.node_addr)->datanode_chain;
            }
            else if(mode == FILE_APPEND) {
                file->ramfs_current_datanode = (uint32_t)(ramfs_get_last_datanote_of_chain(
                    ((ramfs_node_t*)node->ramfs_node.node_addr)->datanode_chain)
                );
            }
            break;
        default:
            return ERR_FS_NOT_SUPPORTED;
    }

    node->accessed_timestamp = time(NULL);

    return ERR_FS_SUCCESS;
}

FS_ERR file_read(FILE* file, uint8_t* buffer, size_t size) {
    if(file->mode != FILE_READ) return ERR_FS_FAILED;

    if(file->position == file->node->size) return ERR_FS_EOF;

    FS_ERR err;
    switch(file->node->fs->type) {
        case FS_FAT32:
            err = fat32_read(file, buffer, size);
            break;
        case FS_RAMFS:
            err = ramfs_read(file, buffer, size);
            break;
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
    if(err != ERR_FS_SUCCESS && err != ERR_FS_EOF) return err;

    file->position += size;
    if(file->position > file->node->size) file->position = file->node->size;
    return err;

}

FS_ERR file_write(FILE* file, uint8_t* buffer, size_t size) {
    if(file->mode == FILE_READ) return ERR_FS_FAILED;

    FS_ERR err;
    switch(file->node->fs->type) {
        case FS_FAT32:
            err = fat32_write(file, buffer, size);
            break;
        case FS_RAMFS:
            err = ramfs_write(file, buffer, size);
            break;
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
    if(err) return err;

    file->position += size;
    file->node->size += size;
    file->node->modified_timestamp = time(NULL);
    return ERR_FS_SUCCESS;
}

FS_ERR file_close(FILE* file) {
    switch (file->node->fs->type) {
        case FS_FAT32:
            fat32_update_entry(file->node);
            break;
        case FS_RAMFS:
            ramfs_update_entry(file->node);
            break;
        default:
            break;
    }

    return ERR_FS_SUCCESS;
}
