#include <semaphore.h>

semaphore_t* semaphore_create(unsigned max_count) {
    semaphore_t* ret = (semaphore_t*)kmalloc(sizeof(semaphore_t));

    if(ret) {
        ret->max_count = max_count;
        ret->current_count = 0;
        ret->waiting_queue.top = NULL;
        ret->waiting_queue.bottom = NULL;
        ret->waiting_queue.size = 0;
        atomic_flag_clear(&ret->lock);
    }

    return ret;
}

uint32_t semaphore_acquire(regs_t* regs, semaphore_t* semaphore) {
    spinlock_acquire(&semaphore->lock);
    if(semaphore->current_count < semaphore->max_count) {
        semaphore->current_count++;
        spinlock_release(&semaphore->lock);
        return (uint32_t)regs;
    } else {
        process_t* current_process = scheduler_get_current();
        current_process->state = PROCESS_STATE_BLOCK;
        process_queue_push(&semaphore->waiting_queue, current_process);

        spinlock_release(&semaphore->lock);
        scheduler_to_next_process(regs, false);
        return scheduler_get_current()->saved_esp;
    }
}

void semaphore_release(semaphore_t* semaphore) {
    spinlock_acquire(&semaphore->lock);
    if(semaphore->waiting_queue.size) {
        process_t* proc = process_queue_pop(&semaphore->waiting_queue);
        proc->state = PROCESS_STATE_READY;
        scheduler_push_ready(proc);
    } else semaphore->current_count--;
    spinlock_release(&semaphore->lock);
}
