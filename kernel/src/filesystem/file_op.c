#include "filesystem.h"

#include "string.h"

#define COPY_SIZE 512

static FS_ERR universal_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, char* new_name) {
    FS_ERR touch_err = fs_touch(new_parent, new_name, copied);
    if(touch_err) return touch_err;

    file_description_t src_file;
    FS_ERR src_open_err = file_open(&src_file, node, "r");
    if(src_open_err) return src_open_err;

    file_description_t dst_file;
    FS_ERR dst_open_err = file_open(&dst_file, copied, "w");
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

FS_ERR fs_read_dir(directory_iterator_t* diriter, fs_node_t* ret_node) {
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

    if(node->fs->type == FS_RAMFS)
        return ramfs_universal_copy(node, new_parent, copied, new_name);
    return universal_copy(node, new_parent, copied, new_name);
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

FS_ERR file_reset(fs_node_t* node) {
    switch(node->fs->type) {
        case FS_RAMFS:
            return ramfs_file_reset(node);
        case FS_FAT32:
            return fat32_file_reset(node);
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
}

FS_ERR file_open(file_description_t* file, fs_node_t* node, char* modestr) {
    if(FS_NODE_IS_DIR(*node)) return ERR_FS_NOT_FILE;

    int mode = 0;

    if(modestr[0] == 'w') {
        mode = FILE_WRITE;
        file_reset(node);
    }
    else if(modestr[0] == 'r') {
        mode = FILE_READ;
    }
    else if(modestr[0] == 'a') {
        mode = FILE_APPEND;
    }
    else return ERR_FS_FAILED;

    if(modestr[1] != '\0') {
        if(modestr[1] == '+') {
            if(mode != FILE_READ)
                mode |= FILE_READ;
            else
                mode |= FILE_WRITE;
        }
        else return ERR_FS_FAILED;
    }

    file->node = node;
    file->mode = mode;
    file->position = 0;

    switch(node->fs->type) {
        case FS_RAMFS:
            if(mode & FILE_WRITE || mode & FILE_READ) {
                file->ramfs.current_datanode = (uint32_t)((ramfs_node_t*)node->ramfs.node_addr)->datanode_chain;
            }
            if(mode & FILE_APPEND) {
                file->ramfs.last_datanode = (uint32_t)(ramfs_get_last_datanote_of_chain(
                    ((ramfs_node_t*)node->ramfs.node_addr)->datanode_chain)
                );
            }
            break;
        case FS_FAT32:
            if(mode & FILE_WRITE || mode & FILE_READ) {
                file->fat32.current_cluster = node->fat32.start_cluster;
            }
            if(mode & FILE_APPEND) {
                file->fat32.last_cluster = fat32_get_last_cluster_of_chain(
                    node->fs,
                    node->fat32.start_cluster
                );
            }
            break;
        default:
            return ERR_FS_NOT_SUPPORTED;
    }

    node->accessed_timestamp = time(NULL);

    return ERR_FS_SUCCESS;
}

FS_ERR file_seek(file_description_t* file, size_t pos) {
    if(file->mode & FILE_APPEND)
        return ERR_FS_FAILED;

    switch(file->node->fs->type) {
        case FS_RAMFS:
            return ramfs_seek(file, pos);
        case FS_FAT32:
            return fat32_seek(file, pos);
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
}

FS_ERR file_read(file_description_t* file, uint8_t* buffer, size_t size) {
    if(!(file->mode & FILE_READ)) return ERR_FS_FAILED;

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

FS_ERR file_write(file_description_t* file, uint8_t* buffer, size_t size) {
    if(!(file->mode & FILE_WRITE) && !(file->mode & FILE_APPEND))
        return ERR_FS_FAILED;

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

    if(file->mode & FILE_APPEND) {
        file->node->size += size;
    }
    else {
        file->position += size;
        if(file->position > file->node->size)
            file->node->size = file->position;
    }
    file->node->modified_timestamp = time(NULL);
    return ERR_FS_SUCCESS;
}

FS_ERR file_close(file_description_t* file) {
    switch(file->node->fs->type) {
        case FS_RAMFS:
            return ramfs_update_entry(file->node);
        case FS_FAT32:
            return fat32_update_entry(file->node);
        default:
            return ERR_FS_SUCCESS;
    }
}
