#include "filesystem.h"
#include "mem.h"

// a virtual fs which use system heap as storage
// each ramnode is a linked list node
// that point to next part of a file (if it is a file)
// or contain ramnode entry (if it is a dir)
// kinda like FAT fs but do not use a allocation table

// each ramnode entry is a address that point to that node location
// if the address is 0 then there is no entry left

// node name will be set directly in fs_node_t

ramnode_t* ramfs_rootnode() {
    ramnode_t* root = (ramnode_t*)kmalloc(sizeof(ramnode_t) + sizeof(uint32_t) * 32);
    if(!root) return NULL;

    root->next = NULL;
    root->creation_milisecond = (clock() % CLOCKS_PER_SEC) * 1000 / CLOCKS_PER_SEC;
    root->creation_timestamp = time(NULL);
    root->modified_timestamp = 0;
    root->accessed_timestamp = 0;

    root->name_length = 1; // "/"
    root->flags = RAMNODE_FLAG_DIRECTORY;

    return root;
}

void ramfs_add_entry(ramnode_t* node, char* name);
void ramfs_remove_entry(ramnode_t* node, ramnode_t* remove_node, bool remove_content);
