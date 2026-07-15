#include "sys/filesystem.h"
#include <filesystem.h>

#include <string.h>

#define UNIVERSAL_COPY_SIZE 512

// copy file from any filesystem to another filesystem
static FS_ERR universal_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, const char* new_name) {
    FS_ERR create_err = node_create(new_parent, new_name, copied);
    if(create_err) return create_err;

    file_description_t src_file;
    FS_ERR src_open_err = file_open(&src_file, node, FILE_OPEN_READ);
    if(src_open_err) return src_open_err;

    file_description_t dst_file;
    FS_ERR dst_open_err = file_open(&dst_file, copied, FILE_OPEN_WRITE | FILE_OPEN_CREATE);
    if(dst_open_err) return dst_open_err;

    FS_ERR final_err = ERR_FS_SUCCESS;

    unsigned remain_size = node->size;
    uint8_t buffer[UNIVERSAL_COPY_SIZE];
    while(remain_size) {
        unsigned write_size = (remain_size >= UNIVERSAL_COPY_SIZE) ? UNIVERSAL_COPY_SIZE : remain_size;
        size_t actual_read_size;
        final_err = file_read(&src_file, buffer, write_size, &actual_read_size);
        if(final_err != ERR_FS_SUCCESS && final_err != ERR_FS_EOF)
            break;

        size_t actual_write_size;
        if((final_err = file_write(&dst_file, buffer, actual_read_size, &actual_write_size)))
            break;

        remain_size -= actual_write_size;
    }

    node_sync(copied);
    if(final_err) return final_err;

    return ERR_FS_SUCCESS;
}

FS_ERR node_setup_directory_iterator(directory_iterator_t* diriter, fs_node_t* node) {
    if(node->fs->setup_directory_iterator)
        return node->fs->setup_directory_iterator(diriter, node);
    return ERR_FS_NOT_SUPPORTED;
}

FS_ERR node_iterate_directory(directory_iterator_t* diriter, fs_node_t* ret_node) {
    if(diriter->node->fs->iterate_directory)
        return diriter->node->fs->iterate_directory(diriter, ret_node);
    return ERR_FS_NOT_SUPPORTED;
}

FS_ERR node_find(fs_node_t* parent, const char* nodename, fs_node_t* ret_node) {
    if(!FS_NODE_IS_DIR(parent)) return ERR_FS_NOT_DIR;

    FS_ERR last_err;
    ret_node->flags = 0;

    directory_iterator_t diriter;
    FS_ERR diriter_err = node_setup_directory_iterator(&diriter, parent);
    if(diriter_err) return diriter_err;

    int (*cmpcmd)(const char*, const char*) = strcmp;
    if(parent->fs->type == FS_FAT32) cmpcmd = strcmp_case_insensitive;

    while(!(last_err = node_iterate_directory(&diriter, ret_node))) {
        if(!cmpcmd(nodename, ret_node->name))
            return ERR_FS_SUCCESS;
    }

    if(last_err != ERR_FS_EOF)
        return last_err;

    return ERR_FS_NOT_FOUND;
}

// make a directory in parent node
FS_ERR node_mkdir(fs_node_t* parent, const char* name, fs_node_t* new_node) {
    if(!FS_NODE_IS_DIR(parent)) return ERR_FS_NOT_DIR;

    new_node->flags = 0;

    if(parent->fs->mkdir)
        return parent->fs->mkdir(parent, name, 0, new_node);
    return ERR_FS_NOT_SUPPORTED;
}

FS_ERR node_create(fs_node_t* parent, const char* name, fs_node_t* new_node) {
    if(!FS_NODE_IS_DIR(parent)) return ERR_FS_NOT_DIR;

    new_node->flags = 0;

    if(parent->fs->node_create)
        return parent->fs->node_create(parent, name, new_node);
    return ERR_FS_NOT_SUPPORTED;
}

FS_ERR node_remove(fs_node_t* parent, fs_node_t* node) {
    if(!FS_NODE_IS_DIR(parent)) return ERR_FS_NOT_DIR;

    if(parent->fs->remove_entry)
        return parent->fs->remove_entry(parent, node, true);
    return ERR_FS_NOT_SUPPORTED;
}

// set new_name to NULL to reuse the old name
FS_ERR node_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, const char* new_name) {
    if(!FS_NODE_IS_DIR(new_parent)) return ERR_FS_NOT_DIR;
    if(FS_NODE_IS_DIR(node)) return ERR_FS_NOT_FILE;

    if(!new_name) new_name = node->name;

    if(node->fs == new_parent->fs) {
        if(node->fs->node_copy)
            return node->fs->node_copy(node, new_parent, copied, new_name);
        return ERR_FS_NOT_SUPPORTED;
    }

    if(node->fs->type == FS_RAMFS)
        return ramfs_universal_copy(node, new_parent, copied, new_name);
    return universal_copy(node, new_parent, copied, new_name);
}

// move a node to a new parent
// only works when moving to the same disk
// set new_name to NULL to reuse the old name
FS_ERR node_move(fs_node_t* node, fs_node_t* new_parent, const char* new_name) {
    if(!FS_NODE_IS_DIR(new_parent)) return ERR_FS_NOT_DIR;
    if(node->fs != new_parent->fs) return ERR_FS_NOT_SUPPORTED;

    if(!new_name) new_name = node->name;

    if(node->fs->node_move)
        return node->fs->node_move(node, new_parent, new_name);
    return ERR_FS_NOT_SUPPORTED;
}

FS_ERR node_sync(fs_node_t* node) {
    if(node->fs->node_sync)
        return node->fs->node_sync(node);
    return ERR_FS_NOT_SUPPORTED;
}

FS_ERR node_reset(fs_node_t* node) {
    if(node->fs->node_reset)
        return node->fs->node_reset(node);
    return ERR_FS_NOT_SUPPORTED;
}
