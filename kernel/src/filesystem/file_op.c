#include "sys/filesystem.h"
#include <filesystem.h>
#include <timer.h>

#include <stdint.h>
#include <string.h>

static FS_ERR make_diriter_adapter(file_description_t* dir, directory_iterator_t* diriter) {
    diriter->current_index = dir->current_index;
    if(dir->node->fs->make_diriter_adapter) {
        dir->node->fs->make_diriter_adapter(dir, diriter);
        return ERR_FS_SUCCESS;
    }
    return ERR_FS_NOT_SUPPORTED;
}

static FS_ERR save_diriter_adapter(file_description_t* dir, directory_iterator_t* diriter) {
    dir->current_index = diriter->current_index;
    if(dir->node->fs->save_diriter_adapter) {
        dir->node->fs->save_diriter_adapter(dir, diriter);
        return ERR_FS_SUCCESS;
    }
    return ERR_FS_SUCCESS;
}

FS_ERR file_setup_directory_iterator(file_description_t* dir) {
    if(!FS_NODE_IS_DIR(dir->node))
        return ERR_FS_NOT_DIR;

    if(!(dir->mode & FILE_OPEN_READ))
        return ERR_FS_NOT_DIR;

    FS_ERR err;

    directory_iterator_t diriter;
    err = node_setup_directory_iterator(&diriter, dir->node);
    if(err) return err;

    err = save_diriter_adapter(dir, &diriter);
    if(err) return err;

    return ERR_FS_SUCCESS;
}

FS_ERR file_iterate_directory(file_description_t* dir, dirent_t* dirent) {
    if(!FS_NODE_IS_DIR(dir->node))
        return ERR_FS_NOT_DIR;

    if(!(dir->mode & FILE_OPEN_READ))
        return ERR_FS_NOT_DIR;

    FS_ERR err;

    // make an adapter
    directory_iterator_t diriter;
    err = make_diriter_adapter(dir, &diriter);
    if(err) return err;

    fs_node_t ret_node;
    err = node_iterate_directory(&diriter, &ret_node);
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

FS_ERR file_open(file_description_t* file, fs_node_t* node, const file_mode_t mode) {
    if(mode & FILE_OPEN_ONLYDIR && !FS_NODE_IS_DIR(node)) {
        return ERR_FS_NOT_DIR;
    }

    if(mode & FILE_OPEN_WRITE && mode & FILE_OPEN_TRUNCATE) {
        node_reset(file->node);
    }

    file->node = node;
    file->mode = mode;
    file->position = 0;

    if(node->fs->file_open) {
        node->fs->file_open(file, node, mode);
    } else {
        return ERR_FS_NOT_SUPPORTED;
    }

    node->accessed_timestamp = timer_get_current_time();

    return ERR_FS_SUCCESS;
}

// seek to absolute position
FS_ERR file_seek(
    file_description_t* file,
    int64_t offset,
    whence_t whence,
    int64_t* final_position
) {
    // TODO:
    // support a+ mode

    if(file->mode & FILE_OPEN_APPEND)
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
        case SEEK_BOTTOM:
            absolute_position = file->node->size;
            break;
    }

    FS_ERR err;
    if(file->node->fs->file_seek_absolute) {
        err = file->node->fs->file_seek_absolute(file, absolute_position);
    } else {
        err = ERR_FS_NOT_SUPPORTED;
    }

    if(!err && final_position)
        *final_position = file->position;

    return err;
}

FS_ERR file_read(file_description_t* file, uint8_t* buffer, size_t size, size_t* actual_read_size) {
    if(!(file->mode & FILE_OPEN_READ)) return ERR_FS_FAILED;

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
    if(file->node->fs->file_read) {
        err = file->node->fs->file_read(file, buffer, size, actual_read_size);
    } else {
        err = ERR_FS_NOT_SUPPORTED;
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
    if(!(file->mode & FILE_OPEN_WRITE))
        return ERR_FS_FAILED;
    if(FS_NODE_IS_DIR(file->node))
        return ERR_FS_NOT_FILE;

    FS_ERR err;
    if(file->node->fs->file_write) {
        err = file->node->fs->file_write(file, buffer, size, actual_write_size);
    } else {
        err = ERR_FS_NOT_SUPPORTED;
    }
    if(err) return err;

    if(FS_NODE_IS_PIPE(file->node)) {
        file->node->size += (*actual_write_size);
        file->position += (*actual_write_size);
    } else if(file->mode & FILE_OPEN_APPEND) {
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
