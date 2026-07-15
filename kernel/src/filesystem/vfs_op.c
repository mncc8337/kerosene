#include <filesystem.h>
#include <process.h>

#include <stdlib.h>
#include <string.h>

// from vfs.c
extern fs_t* FS;

static bool validate_user_buffer(const process_t* current_process, const void* buf, size_t size) {
    if(!current_process->is_user) return true;

    const char* start = (const char*)buf;
    const char* end = start + size;

    if(end < start) return false;
    if((uint32_t)end > KERNEL_START) return false;

    return true;
}

static bool validate_user_string(const process_t* current_process, const char* str) {
    if(!current_process->is_user) return true;

    if((uint32_t)str >= KERNEL_START) return false;

    const char* p = str;
    while((uint32_t)p < KERNEL_START) {
        if (*p == '\0') return true;
        p++;
    }

    return false;
}

// mode should be only:
// - FILE_OPEN_CREATE to create file if not existed. to create a dir, set is_file to false
// - FILE_OPEN_EXCLUSIVE: yield error when the file is existed
// - FILE_OPEN_ONLYDIR: to force find directory only. also disable file creation
FS_ERR vfs_find_and_create_node(
    const char* path,
    fs_node_t* cwd,
    fs_node_t** ret_node,
    const file_mode_t mode,
    const bool is_file
) {
    bool do_create_node = mode & FILE_OPEN_CREATE && !(mode & FILE_OPEN_ONLYDIR);
    bool fail_if_existed = mode & FILE_OPEN_EXCLUSIVE;

    // disable interrupts because we are heavily
    // modifying the vfs tree
    uint32_t eflags;
    asm volatile("pushf; pop %0; cli" : "=r"(eflags));

    fs_node_t* current_node = cwd;
    fs_node_t* parent_node = current_node->parent;
    if(!parent_node) parent_node = current_node;

    size_t pathlen = strlen(path);
    char* pathcpy_pos = kmalloc(pathlen + 1);
    if(!pathcpy_pos) {
        asm volatile("push %0; popf" : : "r"(eflags));
        return ERR_FS_NOT_ENOUGH_SPACE;
    }

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
    bool create_node_flag = false;
    
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
            new_node->parent = NULL;

            bool is_last_token = (token == NULL || *token == '\0');

            FS_ERR find_err = node_find(parent_node, current_name, new_node);
            if(find_err) {
                if(find_err == ERR_FS_NOT_FOUND && is_last_token) {
                    if(do_create_node) {
                        create_node_flag = true;
                        break;
                    }
                    ret_err = ERR_FS_TARGET_NOT_FOUND;
                    goto nuke_search_stack_and_ret;
                }
                ret_err = find_err;
                goto nuke_search_stack_and_ret;
            }

            // successfully found on disk

            if(fail_if_existed && is_last_token) {
                ret_err = ERR_FS_ENTRY_EXISTED;
                goto nuke_search_stack_and_ret;
            }

            // add to vfs tree
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

    if(create_node_flag) {
        fs_node_t* new_node = (fs_node_t*)kmalloc(sizeof(fs_node_t));
        if(!new_node) {
            ret_err = ERR_FS_NOT_ENOUGH_SPACE;
            goto nuke_search_stack_and_ret;
        }

        FS_ERR op_err;
        if(is_file)
            op_err = node_touch(parent_node, current_name, new_node);
        else
            op_err = node_mkdir(parent_node, current_name, new_node);
                                
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

    if((mode & FILE_OPEN_ONLYDIR) && !FS_NODE_IS_DIR(current_node)) {
        ret_err = ERR_FS_NOT_DIR;
        goto nuke_search_stack_and_ret;
    }

    *ret_node = current_node;
    goto ret; // dont nuke search stack

nuke_search_stack_and_ret:
    // NOTE: do cached discovered nodes instead of cleaving them on site
    if(search_stack_counter > 0) {
        // unlink the first node on the search stack
        if(search_stack[0]->parent) {
            fs_node_t* current_sibling = search_stack[0]->parent->children;
            if(current_sibling == search_stack[0]) {
                search_stack[0]->parent->children = current_sibling->next_sibling;
            } else {
                while(current_sibling->next_sibling != search_stack[0]) {
                    current_sibling = current_sibling->next_sibling;
                }
                current_sibling->next_sibling = search_stack[0]->next_sibling;
            }
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

void vfs_cleanup_node_tree(fs_node_t* start_node) {
    fs_node_t* target = start_node;

    while(target != NULL) {
        if(target->refcount > 0 || target->children != NULL) {
            return;
        }

        fs_node_t* parent = target->parent;
        bool parent_became_empty = false;

        if(parent != NULL) {
            if(parent->children == target) {
                parent->children = target->next_sibling;

                if(parent->children == NULL) {
                    parent_became_empty = true;
                }
            } else {
                fs_node_t* current = parent->children;

                while(current != NULL && current->next_sibling != target) {
                    current = current->next_sibling;
                }

                if(current != NULL) {
                    current->next_sibling = target->next_sibling;
                }
            }
        }

        kfree(target);

        if(parent_became_empty) {
            target = parent;
        } else {
            break;
        }
    }
}

// TODO:
// specify the error when failed

int vfs_open(const char* path, const file_mode_t mode) {
    process_t* proc = scheduler_get_current_process();

    if(!validate_user_string(proc, path))
        return -1;

    // return early, prevent searching for the entire table and found nothing
    // NOTE:
    // this does not guarantee us to find a slot, it only shows that there is
    // some slots available but other processes may take it if we dont fast enough
    if(proc->file_count >= MAX_FILE)
        return -1;

    if(!proc->cwd)
        return -1;

    fs_node_t* node;
    FS_ERR find_err = vfs_find_and_create_node(
        path,
        proc->cwd,
        &node,
        mode,
        true
    );
    if(find_err)
        return -1;

    int file_descriptor = -1;
    file_description_t* fde = NULL;

    // prevent other process to hijack our free slot if there is one
    uint32_t eflags;
    asm volatile("pushf; pop %0; cli" : "=r"(eflags));

    // find a hole to insert new file descriptor
    for(file_descriptor = 0; file_descriptor < MAX_FILE; file_descriptor++) {
        if(proc->file_descriptor_table[file_descriptor].node == NULL) {
            fde = proc->file_descriptor_table + file_descriptor;
            break;
        }
    }

    if(!fde) {
        vfs_cleanup_node_tree(node);
        asm volatile("push %0; popf" : : "r"(eflags));
        return -1;
    }

    FS_ERR open_err = file_open(fde, node, mode);
    if(open_err) {
        fde->node = NULL;
        // NOTE: dont nuke the whole tree after just failing to open a node
        vfs_cleanup_node_tree(node);
        asm volatile("push %0; popf" : : "r"(eflags));
        return -1;
    }

    node->refcount++;
    proc->file_count++;

    asm volatile("push %0; popf" : : "r"(eflags));
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

    uint32_t eflags;
    asm volatile("pushf; pop %0; cli" : "=r"(eflags));

    fde->node->refcount--;
    // NOTE: dont nuke the whole tree after just deleting one node
    vfs_cleanup_node_tree(fde->node);

    fde->node = NULL;
    proc->file_count--;

    asm volatile("push %0; popf" : : "r"(eflags));
}

int vfs_read(int file_descriptor, uint8_t* buffer, size_t size) {
    process_t* proc = scheduler_get_current_process();

    if(!validate_user_buffer(proc, buffer, size))
        return -1;

    if(file_descriptor < 0 || (unsigned)file_descriptor >= MAX_FILE)
        return -1;

    file_description_t* fde = proc->file_descriptor_table + file_descriptor;

    if(fde->node == NULL)
        return -1;

    if(!(fde->mode & FILE_OPEN_READ))
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

int vfs_write(int file_descriptor, const uint8_t* buffer, size_t size) {
    process_t* proc = scheduler_get_current_process();

    if(!validate_user_buffer(proc, buffer, size))
        return -1;

    if(file_descriptor < 0 || (unsigned)file_descriptor >= MAX_FILE)
        return -1;

    file_description_t* fde = proc->file_descriptor_table + file_descriptor;

    if(fde->node == NULL)
        return -1;

    if(!(fde->mode & FILE_OPEN_WRITE))
        return -1;

    size_t write_size;
    FS_ERR write_err = file_write(fde, buffer, size, &write_size);

    if(write_err == ERR_FS_SUCCESS)
        return write_size;
    else
        return -1;
}

int64_t vfs_seek(int file_descriptor, int64_t offset, whence_t whence) {
    process_t* proc = scheduler_get_current_process();

    if(file_descriptor < 0 || (unsigned)file_descriptor >= MAX_FILE)
        return -1;

    file_description_t* fde = proc->file_descriptor_table + file_descriptor;

    if(fde->node == NULL)
        return -1;

    int64_t ret;
    FS_ERR seek_err = file_seek(fde, offset, whence, &ret);
    if(seek_err == ERR_FS_SUCCESS)
        return ret;
    else
        return -1;
}

void vfs_seek_syscall(
    int file_descriptor,
    uint32_t hoff,
    uint32_t loff,
    whence_t whence,
    int64_t* position
) {
    int64_t pos = ((uint64_t)(hoff & 0xffffffff) << 32) | (loff & 0xffffffff);
    int64_t ret = vfs_seek(file_descriptor, pos, whence);
    *position = ret;
}
