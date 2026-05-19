#include <filesystem.h>
#include <process.h>

#include <stdlib.h>
#include <string.h>

// from vfs.c
extern fs_t* FS;

FS_ERR vfs_find_and_create_node(
    const char* path,
    fs_node_t* cwd,
    fs_node_t** ret_node,
    bool create_node,
    bool is_file
) {
    // disable interrupts because we are heavily
    // modifying the vfs tree
    uint32_t eflags;
    asm volatile("pushf; pop %0; cli" : "=r"(eflags));

    fs_node_t* current_node = cwd;
    fs_node_t* parent_node = current_node->parent;
    if(!parent_node) parent_node = current_node;

    size_t pathlen = strlen(path);
    char* pathcpy_pos = kmalloc(pathlen + 1);
    if (!pathcpy_pos) return ERR_FS_NOT_ENOUGH_SPACE;

    memcpy(pathcpy_pos, path, pathlen + 1);
    char* pathcpy = pathcpy_pos;

    FS_ERR ret_err = ERR_FS_SUCCESS;

    const unsigned MAX_ALLOCATION = 512;
    fs_node_t** search_stack = kmalloc(sizeof(fs_node_t*) * MAX_ALLOCATION);
    if(!search_stack) {
        ret_err = ERR_FS_NOT_ENOUGH_SPACE;
        goto ret;
    }
    unsigned search_stack_counter = 0;

    // disk specified
    if(pathcpy[0] == '(') {
        char diskid_string[MAX_DISK_ID_STRLEN + 1];
        bool valid_disk_syntax = false;

        for(unsigned i = 0; i < MAX_DISK_ID_STRLEN; i++) {
            if(pathcpy[i + 1] == ')') {
                diskid_string[i] = '\0';
                valid_disk_syntax = true;
                break;
            }
            diskid_string[i] = pathcpy[i + 1];
        }

        if(!valid_disk_syntax) {
            ret_err = ERR_FS_NOT_SUPPORTED;
            goto ret;
        }

        unsigned diskid = atoi(diskid_string);
        if(FS[diskid].type != FS_EMPTY) {
            current_node = &FS[diskid].root_node;
            parent_node = current_node;
        } else {
            ret_err = ERR_FS_NOT_SUPPORTED;
            goto ret;
        }

        pathcpy += strlen(diskid_string) + 3;
    } else if(pathcpy[0] == '/') {
        current_node = &FS[RAMFS_DISK].root_node;
        parent_node = current_node;
        pathcpy++;
    }

    char* token = pathcpy;
    char* current_name = strtok_r(pathcpy, "/", &token);
    bool create_file_flag = false;
    
    while(current_name) {
        if(!strcmp(current_name, "."))
            goto skip_search;

        // root_node does not have .. dir so we need to handle it separately
        if(current_node->name[0] == '/' && !strcmp(current_name, ".."))
            goto skip_search;

        parent_node = current_node;
        current_node = current_node->children;
        
        while(current_node && strcmp(current_node->name, current_name))
            current_node = current_node->next_sibling;

        if(current_node == NULL) {
            // the node we need to find does not in the node tree yet
            // so we need to find it in the disk

            if(search_stack_counter >= MAX_ALLOCATION) {
                ret_err = ERR_FS_MAX_DEPTH_REACHED;
                goto ret;
            }

            fs_node_t* new_node = (fs_node_t*)kmalloc(sizeof(fs_node_t));
            if(!new_node) {
                ret_err = ERR_FS_NOT_ENOUGH_SPACE;
                goto nuke_search_stack_and_ret;
            }
            search_stack[search_stack_counter++] = new_node;

            FS_ERR find_err = fs_find(parent_node, current_name, new_node);
            if(find_err) {
                if(find_err == ERR_FS_NOT_FOUND && strtok_r(NULL, "/", &token) == NULL) {
                    if(create_node) {
                        create_file_flag = true;
                        break;
                    }
                    ret_err = ERR_FS_TARGET_NOT_FOUND;
                    goto nuke_search_stack_and_ret;
                }
                ret_err = find_err;
                goto nuke_search_stack_and_ret;
            }

            // successfully found on disk, add to vfs tree
            if(!parent_node->children) {
                parent_node->children = new_node;
            } else {
                fs_node_t* current_sibling = parent_node->children;
                while(current_sibling->next_sibling)
                    current_sibling = current_sibling->next_sibling;
                current_sibling->next_sibling = new_node;
            }
            current_node = new_node;
        }

        skip_search:
        current_name = strtok_r(NULL, "/", &token);
    }

    if(create_file_flag) {
        fs_node_t* new_node = (fs_node_t*)kmalloc(sizeof(fs_node_t));
        if(!new_node) {
            ret_err = ERR_FS_NOT_ENOUGH_SPACE;
            goto nuke_search_stack_and_ret;
        }

        FS_ERR op_err;
        if(is_file)
            op_err = fs_touch(parent_node, current_name, new_node);
        else
            op_err = fs_mkdir(parent_node, current_name, new_node);
                                
        if(op_err) {
            kfree(new_node);
            ret_err = op_err;
            goto nuke_search_stack_and_ret;
        }

        // link the new node to the tree
        if(!parent_node->children) {
            parent_node->children = new_node;
        } else {
            fs_node_t* current_sibling = parent_node->children;
            while(current_sibling->next_sibling)
                current_sibling = current_sibling->next_sibling;
            current_sibling->next_sibling = new_node;
        }
        new_node->next_sibling = NULL;
        current_node = new_node;
    }

    *ret_node = current_node;
    goto ret; // dont nuke search stack

nuke_search_stack_and_ret:
    // NOTE: do cached discovered nodes instead of cleaving them on site
    if(search_stack_counter > 0) {
        // unlink the first node on the search stack
        fs_node_t* current_sibling = search_stack[0]->parent->children;
        if(current_sibling == search_stack[0]) {
            search_stack[0]->parent->children = current_sibling->next_sibling;
        } else {
            while(current_sibling->next_sibling != search_stack[0]) {
                current_sibling = current_sibling->next_sibling;
            }
            current_sibling->next_sibling = search_stack[0]->next_sibling;
        }

        // now the whole search stack is unlinked from the vfs tree
        // we can clean it by simply kfree all of them
        // note that this is only possible if we block other processes
        // from creating branches from search_stack's nodes
        // which we have done by disabling interrupts

        for(unsigned i = 0; i < search_stack_counter; i++)
            kfree(search_stack[i]);
    }

ret:
    kfree(search_stack);
    kfree(pathcpy_pos);
    asm volatile("push %0; popf" : : "r"(eflags));
    return ret_err;
}

// FIXME: unroll to iteractive loop
void vfs_cleanup_node_tree(fs_node_t* start_node) {
    if(start_node->refcount > 0)
        return;
    if(start_node->children)
        return;

    // now it is safe to delete this node
    
    // remove node from tree
    fs_node_t* parent_node = start_node->parent;
    if(parent_node->children == start_node) {
        if(start_node->next_sibling)
            parent_node->children = start_node->next_sibling;
        else {
            parent_node->children = NULL;
            // parent directory maybe unused at this point
            vfs_cleanup_node_tree(parent_node);
        }
    } else {
        fs_node_t* current_node = parent_node->children;
        while(current_node->next_sibling != start_node)
            current_node = current_node->next_sibling;
        current_node->next_sibling = start_node->next_sibling;
    }

    kfree(start_node);
}

// TODO:
// specify the error when failed

int vfs_open(const char* path, const char* modestr) {
    process_t* proc = scheduler_get_current_process();

    if(proc->file_count >= MAX_FILE)
        return -1;

    if(!proc->cwd)
        return -1;

    fs_node_t* node;
    bool do_touch_file = (modestr[0] == 'w') || (modestr[0] == 'a');
    FS_ERR find_err = vfs_find_and_create_node(path, proc->cwd, &node, do_touch_file, true);
    if(find_err)
        return -1;

    int file_descriptor = -1;
    file_description_t* fde = NULL;

    // find a hole to insert new file descriptor
    // it should always found one
    // since we must not exceed max file limit at this point
    for(file_descriptor = 0; file_descriptor < MAX_FILE; file_descriptor++) {
        if(proc->file_descriptor_table[file_descriptor].node == NULL) {
            fde = proc->file_descriptor_table + file_descriptor;
            break;
        }
    }

    FS_ERR open_err = file_open(fde, node, modestr);
    if(open_err) {
        fde->node = NULL;
        // NOTE: dont nuke the whole tree after just failing to open a node
        vfs_cleanup_node_tree(node);
        return -1;
    }

    node->refcount++;
    proc->file_count++;

    return file_descriptor;
}

void vfs_close(int file_descriptor) {
    process_t* proc = scheduler_get_current_process();

    if(file_descriptor < 0 || (unsigned)file_descriptor >= MAX_FILE)
        return;

    file_description_t* fde = proc->file_descriptor_table + file_descriptor;

    if(fde->node == NULL)
        return;

    file_sync(fde);

    fde->node->refcount--;
    // NOTE: dont nuke the whole tree after just deleting one node
    vfs_cleanup_node_tree(fde->node);

    fde->node = NULL;
    proc->file_count--;
}

int vfs_read(int file_descriptor, uint8_t* buffer, size_t size) {
    process_t* proc = scheduler_get_current_process();

    // prevent user process from writing into
    // kernel's memory space
    if(proc->is_user) {
        void* start = (void*)buffer;
        void* end = start + size;

        // overflow check
        if(end < start) {
            return -1; // sus size
        }

        if(end > (void*)KERNEL_START) {
            return -1; // leaking to kernel's memspace
        }
    }

    if(file_descriptor < 0 || (unsigned)file_descriptor >= MAX_FILE)
        return -1;

    file_description_t* fde = proc->file_descriptor_table + file_descriptor;

    if(fde->node == NULL)
        return -1;

    if(!(fde->mode & FILE_READ))
        return -1;

    size_t read_size;
    FS_ERR read_err = file_read(fde, buffer, size, &read_size);

    if(read_err == ERR_FS_SUCCESS)
        return read_size;
    else if(read_err == ERR_FS_EOF)
        return 0;
    else
        return -1;
}

int vfs_write(int file_descriptor, uint8_t* buffer, size_t size) {
    process_t* proc = scheduler_get_current_process();

    // prevent user process from writing into
    // kernel's memory space
    if(proc->is_user) {
        void* start = (void*)buffer;
        void* end = start + size;

        // overflow check
        if(end < start) {
            return -1; // sus size
        }

        if(end > (void*)KERNEL_START) {
            return -1; // leaking to kernel's memspace
        }
    }

    if(file_descriptor < 0 || (unsigned)file_descriptor >= MAX_FILE)
        return -1;

    file_description_t* fde = proc->file_descriptor_table + file_descriptor;

    if(fde->node == NULL)
        return -1;

    if(!(fde->mode & FILE_WRITE) && !(fde->mode & FILE_APPEND))
        return -1;

    size_t write_size;
    FS_ERR write_err = file_write(fde, buffer, size, &write_size);

    if(write_err == ERR_FS_SUCCESS)
        return write_size;
    else
        return -1;
}
