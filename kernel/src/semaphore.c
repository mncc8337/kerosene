#include <semaphore.h>
#include <filesystem.h>
#include <process.h>
#include <kutils.h>

void semaphore_init(semaphore_t* sem, uint32_t initial_count) {
    sem->count = initial_count;
    sem->waiting_queue.top = NULL;
    sem->waiting_queue.bottom = NULL;
    sem->waiting_queue.size = 0;
    spinlock_init(&sem->lock);
}

semaphore_t* semaphore_create(uint32_t initial_count) {
    semaphore_t* ret = (semaphore_t*)kmalloc(sizeof(semaphore_t));

    if(ret) semaphore_init(ret, initial_count);
    return ret;
}

uint32_t semaphore_acquire(const regs_t* regs, semaphore_t* semaphore, uint32_t count) {
    spinlock_acquire(&semaphore->lock);
    if(semaphore->count >= count) {
        semaphore->count -= count;
        spinlock_release(&semaphore->lock);
        return (uint32_t)regs;
    } else {
        process_t* current_process = scheduler_get_current();
        current_process->state = PROCESS_STATE_BLOCK;
        current_process->waiting_for_resource_count = count;
        process_queue_push(&semaphore->waiting_queue, current_process);

        spinlock_release(&semaphore->lock);
        return scheduler_to_next_process(regs, false);
    }
}

void semaphore_release(semaphore_t* semaphore, uint32_t count) {
    spinlock_acquire(&semaphore->lock);
    semaphore->count += count;

    while(semaphore->waiting_queue.size > 0) {
        process_t* proc = semaphore->waiting_queue.top;
        if(semaphore->count >= proc->waiting_for_resource_count) {
            semaphore->count -= proc->waiting_for_resource_count;
            process_queue_pop(&semaphore->waiting_queue);
            proc->state = PROCESS_STATE_READY;
            scheduler_push_ready(proc);
        } else {
            break;
        }
    }
    spinlock_release(&semaphore->lock);
}

int semaphore_syscall_create(const char* name, uint32_t initial_count) {
    if(!name) return -1;

    semaphore_t* sem = semaphore_create(initial_count);
    if(!sem) return -1;
    
    process_t* current_process = scheduler_get_current();
    if(!validate_user_string(current_process, name)) {
        kfree(sem);
        return -1;
    }

    fs_node_t* node = NULL;
    FS_ERR find_err = vfs_find_and_create_node(
        name,
        current_process->cwd,
        &node,
        FILE_OPEN_CREATE | FILE_OPEN_EXCLUSIVE,
        0,
        RAMFS_TYPE_SEMAPHORE
    );

    if(find_err) {
        kfree(sem);
        return -1;
    }

    node->ramfs.semaphore = sem;
    node_sync(node); // sync the flags and pointer down to the ramfs structure

    int empty_slot = -1;

    // prevent other process to hijack our free slot if there is one
    uint32_t eflags;
    asm volatile("pushf; pop %0; cli" : "=r"(eflags));
    for(int i = 0; i < MAX_FILE; i++) {
        if(current_process->file_descriptor_table[i].node == NULL) {
            empty_slot = i;
            break;
        }
    }

    if(empty_slot >= 0) {
        FS_ERR open_err = file_open(&current_process->file_descriptor_table[empty_slot], node, FILE_OPEN_READ | FILE_OPEN_WRITE);
        if(!open_err) {
            node->refcount++;
            current_process->file_count++;
            asm volatile("push %0; popf" : : "r"(eflags));
            return empty_slot;
        }
        // failed to open
        current_process->file_descriptor_table[empty_slot].node = NULL;
    }

    asm volatile("push %0; popf" : : "r"(eflags));

    vfs_remove_node(node->parent, node);
    return -1;
}

uint32_t semaphore_syscall_acquire(regs_t* regs, int fd, uint32_t count) {
    if(fd < 0 || fd >= MAX_FILE) {
        regs->eax = -1;
        return (uint32_t)regs;
    }

    process_t* proc = scheduler_get_current();
    file_description_t* fde = &proc->file_descriptor_table[fd];

    if(!fde->node || !(fde->node->fs->type == FS_RAMFS && fde->node->ramfs.type == RAMFS_TYPE_SEMAPHORE) || !fde->node->ramfs.semaphore) {
        regs->eax = -1;
        return (uint32_t)regs;
    }

    regs->eax = 0;
    return semaphore_acquire(regs, fde->node->ramfs.semaphore, count);
}

int semaphore_syscall_release(int fd, uint32_t count) {
    if(fd < 0 || fd >= MAX_FILE) return -1;

    process_t* proc = scheduler_get_current();
    file_description_t* fde = &proc->file_descriptor_table[fd];

    if(!fde->node || !(fde->node->fs->type == FS_RAMFS && fde->node->ramfs.type == RAMFS_TYPE_SEMAPHORE) || !fde->node->ramfs.semaphore)
        return -1;

    semaphore_release(fde->node->ramfs.semaphore, count);
    return 0;
}

uint32_t semaphore_syscall_kacquire(const regs_t* regs, semaphore_t* semaphore, uint32_t count) {
    // only allowed kernel to call this
    if((regs->cs & 0x03) != 0) {
        return scheduler_kill_process(regs, 13);
        return (uint32_t)regs;
    }

    return semaphore_acquire(regs, semaphore, count);
}
