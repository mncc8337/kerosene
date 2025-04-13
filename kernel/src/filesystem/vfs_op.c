#include "filesystem.h"
#include "process.h"

#include "stdlib.h"
#include "string.h"

extern fs_t* FS;

static void cleanup_node_tree(fs_node_t* start_node) {
    // TODO: implement this
}

FS_ERR vfs_find(char* path, fs_node_t* cwd, fs_node_t** ret_node) {
    fs_node_t* current_node = cwd;
    fs_node_t* parent_node = current_node->parent;
    if(!parent_node) parent_node = current_node;

    // disk specified
    if(path[0] == '(') {
        char diskid_string[MAX_DISK_ID_STRLEN + 1];
        for(unsigned i = 0; i <= MAX_DISK_ID_STRLEN; i++) {
            if(path[i + 1] != ')') {
                diskid_string[i] = path[i + 1];
            }
            else {
                diskid_string[i] = '\0';
                break;
            }
        }

        unsigned diskid = atoi(diskid_string);
        if(FS[diskid].type != FS_EMPTY) {
            current_node = &FS[diskid].root_node;
            parent_node = current_node;
        }
        else return ERR_FS_NOT_SUPPORTED;

        path += strlen(diskid_string) + 3;
    }
    else if(path[0] == '/') {
        current_node = &FS[RAMFS_DISK].root_node;
        parent_node = current_node;
        path++;
    }

    // it should not allocate more than 512 nodes right?
    fs_node_t* allocated[512];
    unsigned allocated_counter = 0;

    char* old = path;
    char* current_name = strtok(path, "/", &old);
    while(current_name) {
        if(!strcmp(current_name, "."))
            goto skip;

        // root_node does not has .. dir so we need to handle it separately
        if(current_node->name[0] == '/' && !strcmp(current_name, ".."))
            goto skip;

        parent_node = current_node;
        current_node = current_node->children;
        while(current_node && strcmp(current_node->name, current_name))
            current_node = current_node->next_sibling;

        if(current_node == NULL) {
            // the node we need to find does not in the node tree yet
            // so we find it in the disk
            fs_node_t* new_node = (fs_node_t*)kmalloc(sizeof(fs_node_t));
            if(!new_node) {
                for(unsigned i = 0; i < allocated_counter; i++)
                    kfree(allocated[i]);
                return ERR_FS_NOT_ENOUGH_SPACE;
            }
            allocated[allocated_counter++] = new_node;

            FS_ERR find_err = fs_find(parent_node, current_name, new_node);
            if(find_err) {
                for(unsigned i = 0; i < allocated_counter; i++)
                    kfree(allocated[i]);
                // we should clarify wether the target is not found or the dir in the middle is not found
                if(find_err == ERR_FS_NOT_FOUND && strtok(NULL, "/", &old) == NULL)
                    return ERR_FS_TARGET_NOT_FOUND;
                return find_err;
            }

            // and then add it to the tree
            if(!parent_node->children) {
                parent_node->children = new_node;
            }
            else {
                fs_node_t* current_sibling = parent_node->children;
                while(current_sibling->next_sibling)
                    current_sibling = current_sibling->next_sibling;
                current_sibling->next_sibling = new_node;
            }
            new_node->next_sibling = NULL;

            current_node = new_node;
        }

        skip:
        current_name = strtok(NULL, "/", &old);
    }

    // we should found the node and added it to the tree at this point
    *ret_node = current_node;

    return ERR_FS_SUCCESS;
}

int vfs_open(char* path, char* modestr) {
    process_t* proc = scheduler_get_current_process();

    // NOTE: make sure that proc->cwd has been in the node tree already
    fs_node_t* node;
    FS_ERR find_err = vfs_find(path, proc->cwd, &node);
    if(find_err) {
        if(find_err == ERR_FS_TARGET_NOT_FOUND) {
            // TODO: create a new file
            return -1;
        }
        else return -1;
    }

    int file_descriptor = -1;
    file_description_t* fde;

    // find hole to insert new file descriptor
    for(unsigned i = 0; i < (*proc->file_count); i++) {
        if(proc->file_descriptor_table[i].node == NULL) {
            fde = proc->file_descriptor_table + i;
            file_descriptor = i;
            break;
        }
    }

    if(file_descriptor == -1) {
        // no hole found, add a new entry

        if(*proc->file_count >= MAX_FILE) {
            cleanup_node_tree(node);
            return -1;
        }

        fde = proc->file_descriptor_table + (*proc->file_count);
        file_descriptor = *proc->file_count;
        *proc->file_count += 1;
    }

    FS_ERR open_err = file_open(fde, node, modestr);
    if(open_err) {
        if((unsigned)file_descriptor == *proc->file_count - 1) {
            *proc->file_count -= 1;
        }
        fde->node = NULL;
        cleanup_node_tree(node);
        return -1;
    }

    node->refcount++;

    return file_descriptor;
}

void vfs_close(int file_descriptor) {
    process_t* proc = scheduler_get_current_process();

    if(file_descriptor < 0 || (unsigned)file_descriptor >= *(proc->file_count))
        return;

    file_description_t* fde = &proc->file_descriptor_table[file_descriptor];

    if(fde->node == NULL)
        return;

    file_close(fde);

    fde->node->refcount--;
    if(fde->node->refcount <= 0)
        cleanup_node_tree(fde->node);

    fde->node = NULL;
}

