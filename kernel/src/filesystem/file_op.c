#include "filesystem.h"

#include "string.h"

#define COPY_SIZE 512

static FS_ERR general_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, char* new_name) {
    FS_ERR touch_err = fs_touch(new_parent, new_name, copied);
    if(touch_err) return touch_err;

    FILE src_file;
    FS_ERR src_open_err = file_open(&src_file, node, FILE_READ);
    if(src_open_err) return src_open_err;

    FILE dst_file;
    FS_ERR dst_open_err = file_open(&dst_file, copied, FILE_WRITE);
    if(dst_open_err) return dst_open_err;

    FS_ERR final_err = ERR_FS_SUCCESS;

    unsigned remain_size = node->size;
    uint8_t buffer[COPY_SIZE];
    while(remain_size) {
        unsigned write_size = (remain_size >= COPY_SIZE) ? COPY_SIZE : remain_size;
        final_err = file_read(&src_file, buffer, write_size);
        if(final_err != ERR_FS_SUCCESS && final_err != ERR_FS_EOF)
            break;
        if((final_err = file_write(&dst_file, buffer, write_size)))
            break;

        remain_size -= write_size;
    }

    file_close(&src_file);
    file_close(&dst_file);
    if(final_err) return final_err;

    return ERR_FS_SUCCESS;
}

FS_ERR fs_setup_directory_iterator(directory_iterator_t* diriter, fs_node_t* node) {
    if(!FS_NODE_IS_DIR(*node)) return ERR_FS_NOT_DIR;

    switch(node->fs->type) {
        case FS_RAMFS:
            return ramfs_setup_directory_iterator(diriter, node);
        case FS_FAT32:
            return fat32_setup_directory_iterator(diriter, node);
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
}

FS_ERR fs_read_dir( directory_iterator_t* diriter, fs_node_t* ret_node) {
    switch(diriter->node->fs->type) {
        case FS_RAMFS:
            return ramfs_read_dir(diriter, ret_node);
        case FS_FAT32:
            return fat32_read_dir(diriter, ret_node);
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
}

FS_ERR fs_find(fs_node_t* parent, const char* nodename, fs_node_t* ret_node) {
    if(!FS_NODE_IS_DIR(*parent)) return ERR_FS_NOT_DIR;

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
FS_ERR fs_mkdir(fs_node_t* parent, char* name, fs_node_t* new_node) {
    if(!FS_NODE_IS_DIR(*parent)) return ERR_FS_NOT_DIR;

    new_node->flags = 0;

    switch(parent->fs->type) {
        case FS_RAMFS:
            return ramfs_mkdir(parent, name, 0, new_node);
        case FS_FAT32:
            return fat32_mkdir(parent, name, 0, new_node);
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
}

FS_ERR fs_touch(fs_node_t* parent, char* name, fs_node_t* new_node) {
    if(!FS_NODE_IS_DIR(*parent)) return ERR_FS_NOT_DIR;

    new_node->flags = 0;

    uint32_t file_cluster;
    ramfs_datanode_t* datanode_chain;

    switch(parent->fs->type) {
        case FS_RAMFS:
            datanode_chain = ramfs_allocate_datanodes(1, false);
            if(!datanode_chain) return ERR_FS_NOT_ENOUGH_SPACE;
            return ramfs_add_entry(parent, name, datanode_chain, 0, 0, new_node);
        case FS_FAT32:
            file_cluster = fat32_allocate_clusters(parent->fs, 1, false);
            if(!file_cluster) return ERR_FS_NOT_ENOUGH_SPACE;
            return fat32_add_entry(parent, name, file_cluster, 0, 0, new_node);
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
}

FS_ERR fs_remove(fs_node_t* parent, fs_node_t* node) {
    if(!FS_NODE_IS_DIR(*parent)) return ERR_FS_NOT_DIR;

    switch(parent->fs->type) {
        case FS_RAMFS:
            return ramfs_remove_entry(parent, node, true);
        case FS_FAT32:
            return fat32_remove_entry(parent, node, true);
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
}

// set new_name to NULL to reuse the old name
FS_ERR fs_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, char* new_name) {
    if(!FS_NODE_IS_DIR(*new_parent)) return ERR_FS_NOT_DIR;
    if(FS_NODE_IS_DIR(*node)) return ERR_FS_NOT_FILE;

    if(!new_name) new_name = node->name;

    if(node->fs == new_parent->fs) {
        switch(node->fs->type) {
            case FS_RAMFS:
                return ramfs_copy(node, new_parent, copied, new_name);
            case FS_FAT32:
                return fat32_copy(node, new_parent, copied, new_name);
            default:
                return ERR_FS_NOT_SUPPORTED;
        }
    }

    // handle general case
    return general_copy(node, new_parent, copied, new_name);
}

// move a node to a new parent
// only works when moving to the same disk
// set new_name to NULL to reuse the old name
FS_ERR fs_move(fs_node_t* node, fs_node_t* new_parent, char* new_name) {
    if(!FS_NODE_IS_DIR(*new_parent)) return ERR_FS_NOT_DIR;
    if(node->fs != new_parent->fs) return ERR_FS_NOT_SUPPORTED;

    if(!new_name) new_name = node->name;

    switch(node->fs->type) {
        case FS_RAMFS:
            return ramfs_move(node, new_parent, new_name);
        case FS_FAT32:
            return fat32_move(node, new_parent, new_name);
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
}

FS_ERR file_open(FILE* file, fs_node_t* node, int mode) {
    if(FS_NODE_IS_DIR(*node)) return ERR_FS_NOT_FILE;

    file->node = node;
    file->mode = mode;
    if(mode == FILE_WRITE || mode == FILE_READ)
        file->position = 0;
    else if(mode == FILE_APPEND)
        file->position = node->size;
    else return ERR_FS_NOT_SUPPORTED;

    switch(node->fs->type) {
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
        case FS_RAMFS:
            err = ramfs_read(file, buffer, size);
            break;
        case FS_FAT32:
            err = fat32_read(file, buffer, size);
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
        case FS_RAMFS:
            err = ramfs_write(file, buffer, size);
            break;
        case FS_FAT32:
            err = fat32_write(file, buffer, size);
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
        case FS_RAMFS:
            ramfs_update_entry(file->node);
            break;
        case FS_FAT32:
            fat32_update_entry(file->node);
            break;
        default:
            break;
    }

    return ERR_FS_SUCCESS;
}
