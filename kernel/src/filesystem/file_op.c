#include <filesystem.h>

#include <string.h>

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
    } else if(modestr[0] == 'r') {
        mode = FILE_READ;
    } else if(modestr[0] == 'a') {
        mode = FILE_APPEND;
    } else return ERR_FS_FAILED;

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
    // TODO:
    // support a+ mode

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

FS_ERR file_read(file_description_t* file, uint8_t* buffer, size_t size, size_t* actual_read_size) {
    if(!(file->mode & FILE_READ)) return ERR_FS_FAILED;

    if(file->position == file->node->size) return ERR_FS_EOF;

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
    uint8_t* buffer,
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

    if(!(file->mode & FILE_APPEND)) {
        file->position += (*actual_write_size);
    }

    file->node->size += (*actual_write_size);
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
