#include "filesystem.h"
#include "mem.h"

#include "string.h"

#include "stdio.h"

// a virtual fs which use system heap as storage
// each ramnode is a linked list node
// that point to next part of a file (if it is a file)
// or contain ramnode entry (if it is a dir)
// kinda like FAT fs but do not use a allocation table

// each ramnode entry is a address that point to that node location
// if the address is 0 then there is no entry left
// or if the address is 1 then the entry is empty

// ramnode_t structure
// +----------------+----------------------------+
// | ramnode struct | name (not null terminated) |
// +----------------+----------------------------+

// ramnode_data_t structure
// +---------------------+------+
// | ramnode_data struct | data |
// +---------------------+------+

#define END_ENTRY   0
#define EMPTY_ENTRY 1

#define DIRECTORY_RAMNODE_SIZE (32 * sizeof(uint32_t))

// TODO: check if `name` is overflow
// TODO: make a separate heap

static ramnode_t* new_node(char* name, size_t size, uint32_t flags) {
    unsigned namelen = strlen(name);

    ramnode_t* node = (ramnode_t*)kmalloc(sizeof(ramnode_t) + namelen);
    if(!node) return NULL;

    node->size = (flags & FS_FLAG_DIRECTORY ? 0 : size);
    node->creation_milisecond = (clock() % CLOCKS_PER_SEC) * 1000 / CLOCKS_PER_SEC;
    node->creation_timestamp = time(NULL);
    node->modified_timestamp = 0;
    node->accessed_timestamp = 0;

    node->name_length = namelen;
    node->flags = FS_FLAG_VALID | flags;

    // copy name
    memcpy((void*)node + sizeof(ramnode_t), name, namelen);

    node->data = (ramnode_data_t*)kmalloc(sizeof(ramnode_data_t) + size);
    if(!node->data) {
        kfree((void*)node);
        return NULL;
    }
    node->data->size = size;

    // clear mem if it is a directory
    if(flags & FS_FLAG_DIRECTORY)
        memset((void*)node->data + sizeof(ramnode_data_t), END_ENTRY, size);

    return node;
}

fs_node_t to_fs_node(ramnode_t* ramnode, ramnode_t* parent_ramnode, fs_node_t* parent) {
    fs_node_t node;
    char* name = (void*)ramnode + sizeof(ramnode_t);
    memcpy(node.name, name, ramnode->name_length);
    node.name[ramnode->name_length] = '\0';
    node.fs = parent->fs;
    node.parent_node = parent;
    node.flags = FS_FLAG_VALID | ramnode->flags;
    node.creation_milisecond = ramnode->creation_milisecond;
    node.creation_timestamp = ramnode->creation_timestamp;
    node.modified_timestamp = ramnode->modified_timestamp;
    node.accessed_timestamp = ramnode->accessed_timestamp;
    node.size = ramnode->size;
    
    node.ramfs_node.node_addr = (uint32_t)ramnode;
    node.ramfs_node.parent_node_addr = (uint32_t)parent_ramnode;

    return node;
}

FS_ERR ramfs_read_dir(fs_node_t* parent, bool (*callback)(fs_node_t)) {
    if(!FS_NODE_IS_DIR(*parent)) return ERR_FS_NOT_DIR;

    ramnode_t* parent_ramnode = (ramnode_t*)parent->ramfs_node.node_addr;

    ramnode_data_t* parent_datanode = parent_ramnode->data;
    uint32_t* entry_list = (void*)parent_datanode + sizeof(ramnode_data_t);
    unsigned entry_list_count = parent_datanode->size / sizeof(uint32_t);
    while(true) {
        for(unsigned entry_id = 0; entry_id < entry_list_count; entry_id++) {
            if(entry_list[entry_id] == END_ENTRY) break;
            if(entry_list[entry_id] == EMPTY_ENTRY) continue;

            // run callback
            fs_node_t node = to_fs_node((ramnode_t*)entry_list[entry_id], parent_ramnode, parent);
            if(!callback(node)) return ERR_FS_CALLBACK_STOP;
        }

        // to next ramnode
        parent_datanode = parent_datanode->next;
        if(!parent_datanode) break;

        entry_list = (void*)parent_datanode + sizeof(ramnode_data_t);
        entry_list_count = parent_datanode->size / sizeof(uint32_t);
    }

    return ERR_FS_EXIT_NATURALLY;
}

fs_node_t ramfs_add_entry(fs_node_t* parent, char* name, uint32_t flags, size_t size) {
    fs_node_t node; node.flags = 0;
    if(!FS_NODE_IS_DIR(*parent)) return node;

    ramnode_t* parent_ramnode = (ramnode_t*)parent->ramfs_node.node_addr;
    unsigned empty_entry;
    ramnode_data_t* empty_entry_datanode = 0;

    // search for duptication and for empty space
    ramnode_data_t* parent_datanode = parent_ramnode->data;
    uint32_t* entry_list = (void*)parent_datanode + sizeof(ramnode_data_t);
    unsigned entry_list_count = parent_datanode->size / sizeof(uint32_t);
    while(true) {
        for(unsigned entry_id = 0; entry_id < entry_list_count; entry_id++) {
            if(entry_list[entry_id] == END_ENTRY || entry_list[entry_id] == EMPTY_ENTRY) {
                if(!empty_entry_datanode) {
                    empty_entry = entry_id;
                    empty_entry_datanode = parent_datanode;
                }

                if(entry_list[entry_id] == END_ENTRY) break;
                else continue;
            }

            char* current_ramnode_name = (void*)entry_list[entry_id] + sizeof(ramnode_t);
            if(strcmp(current_ramnode_name, name)) return node;
        }

        // to next ramnode
        parent_datanode = parent_datanode->next;
        if(!parent_datanode) break;

        entry_list = (void*)parent_datanode + sizeof(ramnode_data_t);
        entry_list_count = parent_datanode->size / sizeof(uint32_t);
    }

    ramnode_t* ramnode = new_node(name, size, flags);
    if(!ramnode) return node;

    // add ramnode entry

    parent_datanode = empty_entry_datanode;
    entry_list = (void*)parent_datanode + sizeof(ramnode_data_t);
    entry_list_count = parent_datanode->size / sizeof(uint32_t);

    if(entry_list[empty_entry] == END_ENTRY) {
        if(empty_entry < entry_list_count - 1)
            entry_list[empty_entry + 1] = END_ENTRY;
        else {
            // create new datanode and clear it
            ramnode_data_t* new_datanode = (ramnode_data_t*)kmalloc(sizeof(ramnode_data_t) + DIRECTORY_RAMNODE_SIZE);
            if(!new_datanode) {
                kfree((void*)ramnode);
                return node;
            }
            new_datanode->size = DIRECTORY_RAMNODE_SIZE;
            memset((void*)new_datanode + sizeof(ramnode_data_t), END_ENTRY, DIRECTORY_RAMNODE_SIZE);
        }
    }
    entry_list[empty_entry] = (uint32_t)ramnode;

    node = to_fs_node(ramnode, parent_ramnode, parent);
    return node;
}

void ramfs_remove_entry(fs_node_t* node, ramnode_t* remove_node, bool remove_content);

FS_ERR ramfs_init(fs_t* fs) {
    ramnode_t* root_ramnode = new_node("/", DIRECTORY_RAMNODE_SIZE, FS_FLAG_DIRECTORY);
    if(!root_ramnode) return ERR_FS_FAILED;

    // fs->partition;
    fs->type = FS_RAMFS;
    fs->root_node.fs = fs;
    fs->root_node.ramfs_node.parent_node_addr = 0;
    fs->root_node.ramfs_node.node_addr = (uint32_t)root_ramnode;
    fs->root_node.parent_node = NULL;
    fs->root_node.flags = FS_FLAG_VALID | FS_FLAG_DIRECTORY;
    fs->root_node.name[0] = '/';
    fs->root_node.name[1] = '\0';

    return ERR_FS_SUCCESS;
}
