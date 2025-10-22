#include "filesystem.h"
#include "mem.h"

#include "string.h"

// a virtual fs which use system heap as its storage
// each ramnode contains metadata of a file/directory
// and a pointer to a datanodes chain (which contains the file/directory's data)
// each datanode points to the next datanode in the chain
// kinda like FAT fs but do not use a allocation table

// entry name is located right after ramnode structure
// data is located right after datanode structure

// datanode of a directory contains ramnode entries
// each ramnode entry is a address that point to its ramnode location
// if the address is END_ENTRY then there is no entry left
// or if the address is EMPTY_ENTRY then the entry is empty

// ramfs_node_t structure
// +----------------+------------------------+
// | ramnode struct | name (null terminated) |
// +----------------+------------------------+
// name_length property of ramfs_node_t does not account for the null terminating character

// ramfs_datanode_t structure
// +---------------------+
// | ramnode_data struct |
// +---------------------+

#define END_ENTRY   0
#define EMPTY_ENTRY 1

#define DIRECTORY_ENTRY_COUNT (RAMFS_DATANODE_SIZE / sizeof(uint32_t))

static heap_t* rheap;

static void* rmalloc(size_t size) {
    return heap_alloc(rheap, size, false);
}

static ramfs_node_t* create_new_node(char* name, uint32_t flags, size_t size, ramfs_datanode_t* datanode_chain) {
    unsigned namelen = strlen(name);

    if(namelen + 1 > FILENAME_LIMIT) {
        namelen = FILENAME_LIMIT - 1;
        name[FILENAME_LIMIT] = '\0';
    }

    ramfs_node_t* node = (ramfs_node_t*)rmalloc(sizeof(ramfs_node_t) + namelen + 1);
    if(!node) return NULL;

    node->size = size;
    node->creation_milisecond = (clock() % CLOCKS_PER_SEC) * 1000 / CLOCKS_PER_SEC;
    node->creation_timestamp = time(NULL);
    node->modified_timestamp = 0;
    node->accessed_timestamp = 0;

    node->name_length = namelen;
    node->flags = flags;

    // copy name
    memcpy((void*)node + sizeof(ramfs_node_t), name, namelen + 1);

    node->datanode_chain = datanode_chain;

    return node;
}

static void to_fs_node(ramfs_node_t* ramnode, fs_node_t* parent, fs_node_t* node) {
    char* name = (void*)ramnode + sizeof(ramfs_node_t);
    memcpy(node->name, name, ramnode->name_length + 1);
    node->fs = parent->fs;
    node->parent = parent;
    node->children = NULL;
    node->next_sibling = NULL;
    node->flags = ramnode->flags;
    node->creation_milisecond = ramnode->creation_milisecond;
    node->creation_timestamp = ramnode->creation_timestamp;
    node->modified_timestamp = ramnode->modified_timestamp;
    node->accessed_timestamp = ramnode->accessed_timestamp;
    node->size = ramnode->size;
    node->refcount = 0;
    
    node->ramfs.node_addr = (uint32_t)ramnode;
}

static void remove_datanode_chain(ramfs_datanode_t* node) {
    ramfs_datanode_t* current_datanode = node;

    while(current_datanode) {
        ramfs_datanode_t* saved = current_datanode;
        current_datanode = current_datanode->next;
        kfree((void*)saved);
    }
}

static ramfs_datanode_t* copy_datanode_chain(ramfs_datanode_t* datanode) {
    ramfs_datanode_t* current_datanode = datanode;
    ramfs_datanode_t* ret = NULL;

    ramfs_datanode_t* copied = NULL;
    while(current_datanode) {
        if(copied) {
            copied->next = (ramfs_datanode_t*)rmalloc(sizeof(ramfs_datanode_t));
            copied = copied->next;
        } else {
            copied = (ramfs_datanode_t*)rmalloc(sizeof(ramfs_datanode_t));
            ret = copied;
        }
        if(!copied) return NULL; // OOM
        copied->next = NULL;

        memcpy(copied->data, current_datanode->data, RAMFS_DATANODE_SIZE);
        current_datanode = current_datanode->next;
    }

    return ret;
}

static void fix_empty_entries(ramfs_node_t* node) {
    ramfs_datanode_t* current_datanode = node->datanode_chain;

    ramfs_datanode_t* start_node = 0;
    unsigned first_empty_entry_id = 0;

    ramfs_datanode_entry_t* entry_list = (void*)current_datanode->data;

    while(true) {
        for(unsigned entry_id = 0; entry_id < DIRECTORY_ENTRY_COUNT; entry_id++) {
            if(entry_list[entry_id] == END_ENTRY)
                goto start_deleting;

            if(entry_list[entry_id] == EMPTY_ENTRY) {
                if(!start_node) {
                    start_node = current_datanode;
                    first_empty_entry_id = entry_id;
                }
            } else start_node = 0;
        }

        // to next ramnode
        current_datanode = current_datanode->next;
        if(!current_datanode) break;

        entry_list = (uint32_t*)current_datanode->data;
    }

    start_deleting:
    if(!start_node) return;

    // clear empty entry in start node
    entry_list = (void*)start_node->data;
    for(unsigned i = first_empty_entry_id; i < DIRECTORY_ENTRY_COUNT; i++)
        entry_list[i] = END_ENTRY;

    remove_datanode_chain(start_node->next);
    start_node->next = NULL;
}

static FS_ERR read_file(
    ramfs_datanode_t** start_datanode,
    uint8_t* buffer,
    size_t size,
    size_t* actual_read_size,
    int data_offset
) {
    *actual_read_size = 0;

    // make sure that data_offset is less than RAMFS_DATANODE_SIZE
    while(data_offset >= RAMFS_DATANODE_SIZE) {
        if(!(*start_datanode)->next)
            return ERR_FS_EOF;
        *start_datanode = (*start_datanode)->next;

        data_offset -= RAMFS_DATANODE_SIZE;
    }

    if(data_offset > 0) {
        unsigned read_size = RAMFS_DATANODE_SIZE - data_offset;
        bool read_all = (size <= read_size);

        memcpy(buffer, (*start_datanode)->data + data_offset, read_all ? size : read_size);

        if(read_all) {
            *actual_read_size += size;
            return ERR_FS_SUCCESS;
        }

        size -= read_size;
        buffer += read_size;
        *actual_read_size += read_size;

        if(!(*start_datanode)->next)
            return ERR_FS_EOF;
        *start_datanode = (*start_datanode)->next;
    }
    // now we can ignore data_offset

    while(true) {
        unsigned read_size = (size < RAMFS_DATANODE_SIZE) ? size : RAMFS_DATANODE_SIZE;
        memcpy(buffer, (*start_datanode)->data, read_size);

        size -= read_size;
        buffer += read_size;
        *actual_read_size += read_size;
        if(size == 0) break;

        if(!(*start_datanode)->next)
            return ERR_FS_EOF;
        *start_datanode = (*start_datanode)->next;
    }

    return ERR_FS_SUCCESS;
}

static FS_ERR write_file(
    ramfs_datanode_t** start_datanode,
    uint8_t* buffer,
    size_t size,
    size_t* actual_write_size,
    int data_offset
) {
    // basically the same as read_file
    // but change the direction of memcpy
    // and add more datanode when not enough

    *actual_write_size = 0;

    while(data_offset >= RAMFS_DATANODE_SIZE) {
        if(!(*start_datanode)->next) {
            (*start_datanode)->next = ramfs_allocate_datanodes(data_offset / RAMFS_DATANODE_SIZE, false);
            if(!(*start_datanode)->next) return ERR_FS_NOT_ENOUGH_SPACE;
        }
        *start_datanode = (*start_datanode)->next;

        data_offset -= RAMFS_DATANODE_SIZE * (unsigned)(data_offset / RAMFS_DATANODE_SIZE);
    }

    if(data_offset > 0) {
        unsigned full_write_size = RAMFS_DATANODE_SIZE - data_offset;
        unsigned write_size = (size <= full_write_size) ? size : full_write_size;

        memcpy((*start_datanode)->data + data_offset, buffer, write_size);

        size -= write_size;
        buffer += write_size;
        *actual_write_size += write_size;

        if(size == 0)
            return ERR_FS_SUCCESS;

        if(!(*start_datanode)->next) {
            (*start_datanode)->next = ramfs_allocate_datanodes(1, false);
            if(!(*start_datanode)->next) return ERR_FS_NOT_ENOUGH_SPACE;
        }
        *start_datanode = (*start_datanode)->next;
    }

    int estimated_datanode = size / RAMFS_DATANODE_SIZE;
    if(estimated_datanode > 0) {
        (*start_datanode)->next = ramfs_allocate_datanodes(estimated_datanode, false);
        if(!(*start_datanode)->next) return ERR_FS_NOT_ENOUGH_SPACE;
    }

    while(true) {
        unsigned write_size = (size < RAMFS_DATANODE_SIZE) ? size : RAMFS_DATANODE_SIZE;
        memcpy((*start_datanode)->data, buffer, write_size);

        size -= write_size;
        buffer += write_size;
        *actual_write_size += write_size;
        if(size == 0) break;

        // this should not happen
        if(!(*start_datanode)->next)
            return ERR_FS_EOF;
        *start_datanode = (*start_datanode)->next;
    }

    return ERR_FS_SUCCESS;
}

ramfs_datanode_t* ramfs_allocate_datanodes(size_t count, bool clear) {
    if(count == 0) return NULL;

    ramfs_datanode_t* start_node = (ramfs_datanode_t*)rmalloc(sizeof(ramfs_datanode_t));
    if(!start_node) return NULL;
    if(clear) memset((void*)start_node->data, 0, RAMFS_DATANODE_SIZE);
    count--;

    // special case
    if(count == 0) {
        start_node->next = NULL;
        return start_node;
    }

    ramfs_datanode_t* current_node = start_node;

    while(count) {
        current_node->next = (ramfs_datanode_t*)rmalloc(sizeof(ramfs_datanode_t));
        if(!current_node->next) {
            remove_datanode_chain(start_node);
            return 0;
        }
        if(clear) memset((void*)current_node->data, 0, RAMFS_DATANODE_SIZE);
        current_node = current_node->next;
        current_node->next = NULL;
        count--;
    }

    return start_node;
}

ramfs_datanode_t* ramfs_get_last_datanote_of_chain(ramfs_datanode_t* datanode) {
    while(datanode->next)
        datanode = datanode->next;
    return datanode;
}

FS_ERR ramfs_setup_directory_iterator(directory_iterator_t* diriter, fs_node_t* node) {
    diriter->node = node;
    diriter->current_index = -1;
    diriter->ramfs.current_datanode = (uint32_t)((ramfs_node_t*)node->ramfs.node_addr)->datanode_chain;

    return ERR_FS_SUCCESS;
}

FS_ERR ramfs_read_dir(directory_iterator_t* diriter, fs_node_t* ret_node) {
    ramfs_datanode_t* datanode = (ramfs_datanode_t*)diriter->ramfs.current_datanode;
    ramfs_datanode_entry_t* entry_list = (void*)datanode->data;

    diriter->current_index++;
    if(diriter->current_index == DIRECTORY_ENTRY_COUNT) {
        datanode = datanode->next;
        if(datanode == NULL) return ERR_FS_EOF;
        entry_list = (void*)datanode->data;
        diriter->current_index = 0;
    }

    while(entry_list[diriter->current_index] == EMPTY_ENTRY) {
        diriter->current_index++;

        if(diriter->current_index == DIRECTORY_ENTRY_COUNT) {
            datanode = datanode->next;
            if(datanode == NULL) return ERR_FS_EOF;
            entry_list = (void*)datanode->data;
            diriter->current_index = 0;
        }
    }

    if(entry_list[diriter->current_index] == END_ENTRY)
        return ERR_FS_EOF;

    to_fs_node((ramfs_node_t*)entry_list[diriter->current_index], diriter->node, ret_node);
    diriter->ramfs.current_datanode = (uint32_t)datanode;
    return ERR_FS_SUCCESS;
}

FS_ERR ramfs_add_entry(fs_node_t* parent, char* name, ramfs_datanode_t* datanode_chain, uint32_t flags, size_t size, fs_node_t* new_node) {
    new_node->flags = 0;

    ramfs_node_t* parent_ramnode = (ramfs_node_t*)parent->ramfs.node_addr;
    unsigned empty_entry = 0;
    ramfs_datanode_t* empty_entry_datanode = 0;

    // search for duptication and for empty space
    ramfs_datanode_t* parent_datanode = parent_ramnode->datanode_chain;
    ramfs_datanode_entry_t* entry_list = (void*)parent_datanode->data;
    ramfs_datanode_t* final_datanode = 0;
    while(true) {
        for(unsigned entry_id = 0; entry_id < DIRECTORY_ENTRY_COUNT; entry_id++) {
            if(entry_list[entry_id] == END_ENTRY || entry_list[entry_id] == EMPTY_ENTRY) {
                if(!empty_entry_datanode) {
                    empty_entry = entry_id;
                    empty_entry_datanode = parent_datanode;
                }

                if(entry_list[entry_id] == END_ENTRY) break;
                else continue;
            }

            char* current_ramnode_name = (void*)entry_list[entry_id] + sizeof(ramfs_node_t);
            if(!strcmp(current_ramnode_name, name)) return ERR_FS_ENTRY_EXISTED;
        }

        // to next ramnode
        final_datanode = parent_datanode;
        parent_datanode = parent_datanode->next;
        if(!parent_datanode) break;

        entry_list = (void*)parent_datanode->data;
    }

    ramfs_node_t* ramnode = create_new_node(name, flags, size, datanode_chain);
    if(!ramnode) return ERR_FS_NOT_ENOUGH_SPACE;

    // add ramnode entry

    parent_datanode = empty_entry_datanode;
    entry_list = (void*)parent_datanode->data;

    if(entry_list[empty_entry] == END_ENTRY) {
        if(empty_entry < DIRECTORY_ENTRY_COUNT - 1)
            entry_list[empty_entry + 1] = END_ENTRY;
        else {
            // create a new datanode and clear it
            ramfs_datanode_t* new_datanode = (ramfs_datanode_t*)rmalloc(sizeof(ramfs_datanode_t));
            if(!new_datanode) {
                kfree((void*)ramnode);
                return ERR_FS_NOT_ENOUGH_SPACE;
            }
            new_datanode->next = NULL;
            memset((void*)new_datanode->data, END_ENTRY, RAMFS_DATANODE_SIZE);
            final_datanode->next = new_datanode;
        }
    }
    entry_list[empty_entry] = (ramfs_datanode_entry_t)ramnode;

    to_fs_node(ramnode, parent, new_node);
    new_node->size = size;
    return ERR_FS_SUCCESS;
}

FS_ERR ramfs_remove_entry(fs_node_t* parent, fs_node_t* remove_node, bool remove_content) {
    if(!strcmp(remove_node->name, ".") || !strcmp(remove_node->name, ".."))
        return ERR_FS_FAILED;

    ramfs_node_t* parent_ramnode = (ramfs_node_t*)parent->ramfs.node_addr;
    ramfs_node_t* remove_ramnode = (ramfs_node_t*)remove_node->ramfs.node_addr;

    // check if the dir is empty
    if(FS_NODE_IS_DIR(*remove_node) && remove_content) {
        ramfs_datanode_t* datanode = remove_ramnode->datanode_chain;
        ramfs_datanode_entry_t* entry_list = (void*)datanode->data;
        while(true) {
            for(unsigned entry_id = 0; entry_id < DIRECTORY_ENTRY_COUNT; entry_id++) {
                if(entry_list[entry_id] == END_ENTRY) break;
                if(entry_list[entry_id] == EMPTY_ENTRY) return ERR_FS_DIR_NOT_EMPTY;

                ramfs_node_t* ramnode = (ramfs_node_t*)entry_list[entry_id];
                char* name = (void*)ramnode + sizeof(ramfs_node_t);
                if(strcmp(name, ".") && strcmp(name, ".."))
                    return ERR_FS_DIR_NOT_EMPTY;
            }

            // to next ramnode
            datanode = datanode->next;
            if(!datanode) break;

            entry_list = (void*)datanode->data;
        }
    }

    // find the target node in parent_ramnode
    bool found = false;
    unsigned entry_id = 0;
    ramfs_datanode_t* parent_datanode = parent_ramnode->datanode_chain;
    ramfs_datanode_entry_t* entry_list = (void*)parent_datanode->data;
    while(true) {
        for(entry_id = 0; entry_id < DIRECTORY_ENTRY_COUNT; entry_id++) {
            if(entry_list[entry_id] == END_ENTRY) break;
            if(entry_list[entry_id] == EMPTY_ENTRY) continue;

            if(entry_list[entry_id] == (ramfs_datanode_entry_t)remove_ramnode) {
                found = true;
                break;
            }
        }

        if(found) break;

        // to next ramnode
        parent_datanode = parent_datanode->next;
        if(!parent_datanode) break;

        entry_list = (void*)parent_datanode->data;
    }

    if(!found) return ERR_FS_NOT_FOUND;

    // entry_id, entry_list now hold the infomation of
    // the remove entry

    if(remove_content)
        remove_datanode_chain(remove_ramnode->datanode_chain);

    kfree((void*)remove_ramnode);

    // remove entry
    if(entry_id < DIRECTORY_ENTRY_COUNT - 1) {
        if(entry_list[entry_id + 1] == END_ENTRY) {
            entry_list[entry_id] = END_ENTRY;
            
            // there are may be a bunch of empty entries before this so
            if(entry_id == 0 || entry_list[entry_id - 1] == EMPTY_ENTRY)
                fix_empty_entries(parent_ramnode);
        } else entry_list[entry_id] = EMPTY_ENTRY;
    } else {
        // the next datanode maybe cleared/emptied
        // so just set to EMPTY and let the machine does its job
        entry_list[entry_id] = EMPTY_ENTRY;
        fix_empty_entries(parent_ramnode);
    }

    return ERR_FS_SUCCESS;
}

FS_ERR ramfs_update_entry(fs_node_t* node) {
    ramfs_node_t* ramnode = (ramfs_node_t*)node->ramfs.node_addr;

    ramnode->flags = node->flags;
    ramnode->accessed_timestamp = node->accessed_timestamp;
    ramnode->modified_timestamp = node->modified_timestamp;
    ramnode->size = node->size;

    return ERR_FS_SUCCESS;
}

FS_ERR ramfs_mkdir(fs_node_t* parent, char* name, uint32_t flags, fs_node_t* new_node) {
    new_node->flags = 0;

    ramfs_datanode_t* datanode_chain = ramfs_allocate_datanodes(1, true);
    if(!datanode_chain) return ERR_FS_NOT_ENOUGH_SPACE;

    FS_ERR add_entry_err = ramfs_add_entry(parent, name, datanode_chain, FS_FLAG_DIRECTORY | flags, 0, new_node);
    if(add_entry_err) return add_entry_err;

    // add dot dir and dotdot dir

    fs_node_t dotdir;
    FS_ERR dotdir_err = ramfs_add_entry(
        new_node, ".",
        ((ramfs_node_t*)new_node->ramfs.node_addr)->datanode_chain,
        FS_FLAG_DIRECTORY | FS_FLAG_HIDDEN, 0,
        &dotdir
    );
    if(dotdir_err) return dotdir_err;

    fs_node_t dotdotdir;
    FS_ERR dotdotdir_err = ramfs_add_entry(
        new_node, "..",
        ((ramfs_node_t*)parent->ramfs.node_addr)->datanode_chain,
        FS_FLAG_DIRECTORY | FS_FLAG_HIDDEN, 0,
        &dotdotdir
    );
    if(dotdotdir_err) return dotdotdir_err;

    return ERR_FS_SUCCESS;
}

FS_ERR ramfs_move(fs_node_t* node, fs_node_t* new_parent, char* new_name) {
    fs_node_t copied;
    FS_ERR copy_err = ramfs_add_entry(
        new_parent, new_name,
        ((ramfs_node_t*)node->ramfs.node_addr)->datanode_chain,
        node->flags, node->size,
        &copied
    );
    if(copy_err) return copy_err;

    // remove the entry but not the content
    FS_ERR remove_err = ramfs_remove_entry(node->parent, node, false);
    if(remove_err) {
        // TODO: revert adding entry
        return remove_err;
    }

    *node = copied;
    return ERR_FS_SUCCESS;
}

FS_ERR ramfs_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, char* new_name) {
    ramfs_node_t* ramnode = (ramfs_node_t*)(node->ramfs.node_addr);
    ramfs_datanode_t* copied_datanode = copy_datanode_chain(ramnode->datanode_chain);
    if(!copied_datanode) return ERR_FS_NOT_ENOUGH_SPACE;

    FS_ERR err = ramfs_add_entry(
        new_parent,
        new_name,
        copied_datanode,
        node->flags,
        node->size,
        copied
    );
    if(err) return err;

    return ERR_FS_SUCCESS;
}

FS_ERR ramfs_universal_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, char* new_name) {
    FS_ERR touch_err = fs_touch(new_parent, new_name, copied);
    if(touch_err) return touch_err;

    file_description_t dst_file;
    FS_ERR dst_open_err = file_open(&dst_file, copied, "w");
    if(dst_open_err) return dst_open_err;

    ramfs_node_t* ramnode = (ramfs_node_t*)node->ramfs.node_addr;
    ramfs_datanode_t* current_datanode = ramnode->datanode_chain;

    while(current_datanode) {
        unsigned size = RAMFS_DATANODE_SIZE;
        if(!current_datanode->next) size = ramnode->size % RAMFS_DATANODE_SIZE;

        size_t write_size;
        size_t written_size = 0;
        while(written_size < size) {
            FS_ERR write_err = file_write(
                &dst_file,
                current_datanode->data + written_size,
                size - written_size,
                &write_size
            );
            if(write_err && write_err != ERR_FS_EOF) return write_err;
            written_size += write_size;
        }

        current_datanode = current_datanode->next;
    }

    file_close(&dst_file);

    return ERR_FS_SUCCESS;
}

FS_ERR ramfs_file_reset(fs_node_t* node) {
    ramfs_datanode_t* datanode_chain = ((ramfs_node_t*)node->ramfs.node_addr)->datanode_chain;
    remove_datanode_chain(datanode_chain->next);
    datanode_chain->next = NULL;

    node->size = 0;
    return ERR_FS_SUCCESS;
}

FS_ERR ramfs_seek(file_description_t* file, size_t pos) {
    ramfs_datanode_t* current_datanode = ((ramfs_node_t*)file->node->ramfs.node_addr)->datanode_chain;
    size_t current_size = 0;

    if(file->position < pos) {
        current_datanode = (ramfs_datanode_t*)file->ramfs.current_datanode;
        current_size = file->position - (file->position % RAMFS_DATANODE_SIZE);
    }

    while(true) {
        current_size += RAMFS_DATANODE_SIZE;
        if(current_size >= pos) {
            current_size -= RAMFS_DATANODE_SIZE;
            break;
        }
        if(current_datanode->next)
            current_datanode = current_datanode->next;
        else break;
    }

    if(current_size + RAMFS_DATANODE_SIZE < pos)
        file->position = file->node->size;
    else
        file->position = pos;

    file->ramfs.current_datanode = (uint32_t)current_datanode;

    return ERR_FS_SUCCESS;
}

FS_ERR ramfs_read(
    file_description_t* file,
    uint8_t* buffer,
    size_t size,
    size_t* actual_read_size
) {
    int offset = file->position % RAMFS_DATANODE_SIZE;
    if(file->position != 0 && offset == 0)
        offset = RAMFS_DATANODE_SIZE;

    return read_file(
        (ramfs_datanode_t**)&file->ramfs.current_datanode,
        buffer,
        size,
        actual_read_size,
        offset
    );
}

FS_ERR ramfs_write(
    file_description_t* file,
    uint8_t* buffer,
    size_t size,
    size_t* actual_write_size
) {
    int position = file->position;
    ramfs_datanode_t** datanode_ptr = (ramfs_datanode_t**)&file->ramfs.current_datanode;
    if(file->mode & FILE_APPEND) {
        position = file->node->size;
        datanode_ptr = (ramfs_datanode_t**)&file->ramfs.last_datanode;
    }

    int offset = position % RAMFS_DATANODE_SIZE;
    if(position != 0 && offset == 0)
        offset = RAMFS_DATANODE_SIZE;

    return write_file(
        datanode_ptr,
        buffer,
        size,
        actual_write_size,
        offset
    );
}

FS_ERR ramfs_init(fs_t* fs) {
    rheap = heap_new(
        RHEAP_START,
        RHEAP_INITIAL_SIZE,
        RHEAP_MAX_SIZE,
        HEAP_SUPERVISOR
    );

    if(!rheap) return ERR_FS_NOT_ENOUGH_SPACE;

    ramfs_datanode_t* datanode_chain = ramfs_allocate_datanodes(1, true);
    if(!datanode_chain) return ERR_FS_NOT_ENOUGH_SPACE;

    ramfs_node_t* root_ramnode = create_new_node("/", FS_FLAG_DIRECTORY, 0, datanode_chain);
    if(!root_ramnode) return ERR_FS_NOT_ENOUGH_SPACE;

    // fs->partition;
    fs->type = FS_RAMFS;
    fs->root_node.fs = fs;
    fs->root_node.ramfs.node_addr = (uint32_t)root_ramnode;
    fs->root_node.parent = NULL;
    fs->root_node.children = NULL;
    fs->root_node.next_sibling = NULL;
    fs->root_node.flags = FS_FLAG_DIRECTORY;
    fs->root_node.name[0] = '/';
    fs->root_node.name[1] = '\0';
    fs->root_node.refcount = 69420;

    return ERR_FS_SUCCESS;
}
