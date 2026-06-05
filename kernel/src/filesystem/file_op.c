#include "sys/dirent.h"
#include <filesystem.h>
#include <timer.h>

#include <stdint.h>
#include <string.h>

static FS_ERR make_diriter_adapter(file_description_t* dir, directory_iterator_t* diriter) {
    diriter->current_index = dir->current_index;
    switch(dir->node->fs->type) {
        case FS_RAMFS:
             diriter->ramfs.current_datanode = dir->ramfs.current_datanode;
            break;
        case FS_FAT32:
             diriter->fat32.current_cluster = dir->fat32.current_cluster;
            break;
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
    return ERR_FS_SUCCESS;
}

static FS_ERR save_diriter_adapter(file_description_t* dir, directory_iterator_t* diriter) {
    dir->current_index = diriter->current_index;
    switch(dir->node->fs->type) {
        case FS_RAMFS:
            dir->ramfs.current_datanode = diriter->ramfs.current_datanode;
            break;
        case FS_FAT32:
            dir->fat32.current_cluster = diriter->fat32.current_cluster;
            break;
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
    return ERR_FS_SUCCESS;
}

FS_ERR file_setup_directory_iterator(file_description_t* dir) {
    if(!FS_NODE_IS_DIR(dir->node))
        return ERR_FS_NOT_DIR;

    if(dir->mode != FILE_READ)
        return ERR_FS_NOT_DIR;

    FS_ERR err;

    directory_iterator_t diriter;
    err = fs_setup_directory_iterator(&diriter, dir->node);
    if(err) return err;

    err = save_diriter_adapter(dir, &diriter);
    if(err) return err;

    return ERR_FS_SUCCESS;
}

FS_ERR file_iterate_directory(file_description_t* dir, dirent_t* dirent) {
    if(!FS_NODE_IS_DIR(dir->node))
        return ERR_FS_NOT_DIR;

    if(dir->mode != FILE_READ)
        return ERR_FS_NOT_DIR;

    FS_ERR err;

    // make an adapter
    directory_iterator_t diriter;
    err = make_diriter_adapter(dir, &diriter);
    if(err) return err;

    fs_node_t ret_node;
    err = fs_iterate_directory(&diriter, &ret_node);
    if(err) return err;

    err = save_diriter_adapter(dir, &diriter);
    if(err) return err;

    dirent->d_ino = 0; // TODO: figure out what should i put here

    if(FS_NODE_IS_DIR(&ret_node))
        dirent->d_type = DT_DIR;
    else if(FS_NODE_IS_PIPE(&ret_node))
        dirent->d_type = DT_FIFO;
    else
        dirent->d_type = DT_REG;

    strcpy(dirent->d_name, ret_node.name);

    return ERR_FS_SUCCESS;
}

FS_ERR file_reset(file_description_t* file) {
    switch(file->node->fs->type) {
        case FS_RAMFS:
            return ramfs_file_reset(file->node);
        case FS_FAT32:
            return fat32_file_reset(file->node);
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
}

FS_ERR file_open(file_description_t* file, fs_node_t* node, const char* modestr) {
    int mode = 0;

    if(modestr[0] == 'w') {
        mode = FILE_WRITE;
        file_reset(file);
    } else if(modestr[0] == 'r') {
        mode = FILE_READ;
    } else if(modestr[0] == 'a') {
        mode = FILE_APPEND;
    } else return ERR_FS_FAILED;

    if(FS_NODE_IS_DIR(node)) {
        if(mode != FILE_READ) {
            return ERR_FS_NOT_FILE;
        }
    }

    if(modestr[1] != '\0') {
        if(modestr[1] == '+') {
            if(mode != FILE_READ)
                mode |= FILE_READ;
            else
                mode |= FILE_WRITE;
        } else return ERR_FS_FAILED;
    }

    file->node = node;
    file->mode = mode;
    file->position = 0;

    // TODO:
    // clear file content if mode is w or w+

    switch(node->fs->type) {
        case FS_RAMFS:
            if(node->flags & FS_FLAG_MEMORY) {
                break;
            }
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

    node->accessed_timestamp = timer_get_current_time();

    return ERR_FS_SUCCESS;
}

// seek to absolute position
FS_ERR file_seek(
    file_description_t* file,
    int64_t offset,
    int whence,
    int64_t* final_position
) {
    // TODO:
    // support a+ mode

    if(file->mode & FILE_APPEND)
        return ERR_FS_FAILED;

    int64_t absolute_position = 0;

    switch(whence) {
        case SEEK_ABSOLUTE:
            absolute_position = offset;
            break;
        case SEEK_RELATIVE:
            absolute_position = file->position + offset;
            if(absolute_position < 0) {
                absolute_position = 0;
            }
            break;
        case SEEK_END:
            absolute_position = file->node->size;
            break;
    }

    FS_ERR err;

    switch(file->node->fs->type) {
        case FS_RAMFS:
            err = ramfs_seek_absolute(file, absolute_position);
            break;
        case FS_FAT32:
            err = fat32_seek_absolute(file, absolute_position);
            break;
        default:
            err = ERR_FS_NOT_SUPPORTED;
    }

    if(!err && final_position)
        *final_position = file->position;

    return err;
}

FS_ERR file_read(file_description_t* file, uint8_t* buffer, size_t size, size_t* actual_read_size) {
    if(!(file->mode & FILE_READ)) return ERR_FS_FAILED;

    if(FS_NODE_IS_PIPE(file->node)) {
        // pipe mode: node->size = bytes available; empty pipe returns 0, not EOF
        if(file->node->size == 0) {
            *actual_read_size = 0;
            return ERR_FS_SUCCESS;
        }
        if(size > file->node->size)
            size = file->node->size;
        if(file->node->fs->type == FS_RAMFS) {
            return ramfs_pipe_read(file, buffer, size, actual_read_size);
        }
        return ERR_FS_NOT_SUPPORTED;
    }

    if(file->position == file->node->size) return ERR_FS_EOF;

    if(file->position + size > file->node->size) {
        size = file->node->size - file->position;
    }

    FS_ERR err;
    switch(file->node->fs->type) {
        case FS_RAMFS:
            err = ramfs_read(file, buffer, size, actual_read_size);
            break;
        case FS_FAT32:
            err = fat32_read(file, buffer, size, actual_read_size);
            break;
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
    if(err != ERR_FS_SUCCESS && err != ERR_FS_EOF) return err;

    file->position += (*actual_read_size);
    // if(file->position > file->node->size)
    //     file->position = file->node->size;

    return err;

}

FS_ERR file_write(
    file_description_t* file,
    const uint8_t* buffer,
    size_t size,
    size_t* actual_write_size
) {
    if(!(file->mode & FILE_WRITE) && !(file->mode & FILE_APPEND))
        return ERR_FS_FAILED;

    FS_ERR err;
    switch(file->node->fs->type) {
        case FS_RAMFS:
            err = ramfs_write(file, buffer, size, actual_write_size);
            break;
        case FS_FAT32:
            err = fat32_write(file, buffer, size, actual_write_size);
            break;
        default:
            return ERR_FS_NOT_SUPPORTED;
    }
    if(err) return err;

    if(FS_NODE_IS_PIPE(file->node)) {
        file->node->size += (*actual_write_size);
        file->position += (*actual_write_size);
    } else if(file->mode & FILE_APPEND) {
        file->node->size += (*actual_write_size);
        file->position = file->node->size;
    } else {
        file->position += (*actual_write_size);
        if(file->position > file->node->size) {
            file->node->size = file->position;
        }
    }
    file->node->modified_timestamp = timer_get_current_time();
    return ERR_FS_SUCCESS;
}

FS_ERR file_sync(file_description_t* file) {
    switch(file->node->fs->type) {
        case FS_RAMFS:
            return ramfs_update_entry(file->node);
        case FS_FAT32:
            return fat32_update_entry(file->node);
        default:
            return ERR_FS_SUCCESS;
    }
}
