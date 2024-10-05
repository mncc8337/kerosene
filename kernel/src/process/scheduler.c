#include "process.h"
#include "system.h"

#include "string.h"

static process_queue_t ready_queue = PROCESS_QUEUE_INIT;
// this is a linked list sorted by sleep_ticks
// TODO: use a priority queue instead
static process_queue_t sleep_queue = PROCESS_QUEUE_INIT;
static process_queue_t delete_queue = PROCESS_QUEUE_INIT;

static process_t* current_process = NULL;

// FIXME
// currently the only way to get all processes is though process queues
// now i have semaphore implemented and each semaphore have its own process queue
// now to get blocked processes i need to read semaphores' queue
// since semaphores are unmanaged so it is imposible to get all of them

static uint64_t global_sleep_ticks = 0;

static bool process_switched = false;

static void context_switch(regs_t* regs) {
    memcpy(regs, &current_process->regs, sizeof(regs_t));
    vmmngr_switch_page_directory(current_process->page_directory);

    // set err_code to tell that the registers has changed (see syscall.c)
    regs->err_code = 1;
    process_switched = false;
}

static void to_next_process(regs_t* regs, bool add_back) {
    if(!ready_queue.size) return;

    // save registers
    if(regs) memcpy(&current_process->regs, regs, sizeof(regs_t));
    
    if(add_back) {
        current_process->state = PROCESS_STATE_READY;
        process_queue_push(&ready_queue, current_process);
    }

    current_process = process_queue_pop(&ready_queue);
    current_process->state = PROCESS_STATE_ACTIVE;

    process_switched = true;
}

process_t* scheduler_get_current_process() {
    return current_process;
}

process_t* scheduler_get_ready_processes() {
    return ready_queue.top;
}

process_t* scheduler_get_sleep_processes() {
    return sleep_queue.top;
}

void scheduler_add_process(process_t* proc) {
    proc->state = PROCESS_STATE_READY;
    process_queue_push(&ready_queue, proc);
}

// put current process to delete queue, delete it later
void scheduler_kill_process(regs_t* regs) {
    if(current_process->id == 1) return; // avoid deleting kernel process

    // because we are using the stack of the process
    // and process_delete() will free the stack
    // we will postpone the process deleting
    // and do it later when we are using other process's stack
    process_queue_push(&delete_queue, current_process);

    // dont save registers, dont add process back to ready queue
    to_next_process(NULL, false);
    // process switching always happens because there is always a kernel process to switch to
    context_switch(regs);
}

void scheduler_set_sleep(regs_t* regs, unsigned ticks) {
    // set sleep target
    current_process->sleep_ticks = ticks + global_sleep_ticks;

    current_process->state = PROCESS_STATE_SLEEP;
    process_queue_sorted_push(&sleep_queue, current_process, process_sort_by_sleep_ticks);
    // save registers, dont add process back to ready queue
    to_next_process(regs, false);

    if(process_switched) context_switch(regs);
}

void scheduler_switch(regs_t* regs) {
    while(delete_queue.size)
        process_delete(process_queue_pop(&delete_queue));

    if(sleep_queue.size) {
        global_sleep_ticks++;
        while(sleep_queue.top && sleep_queue.top->sleep_ticks <= global_sleep_ticks) {
            process_t* proc = process_queue_pop(&sleep_queue);
            proc->state = PROCESS_STATE_READY;
            process_queue_push(&ready_queue, proc);
        }

        // avoid overflow
        // i mean this solution is suck because if you sleep() for a very long time
        // then global_sleep_ticks will overflow, causing all process in the queue to wake up
        // or did not sleep at all
        // separate ticks into hours/minutes/seconds structure maybe the solution
        if(sleep_queue.size == 0) global_sleep_ticks = 0;
    }

    // switch to other thread if exceeded max runtime
    if(!process_switched && current_process->alive_ticks % PROCESS_ALIVE_TICKS == 0)
        to_next_process(regs, true);

    current_process->alive_ticks++;

    if(process_switched) context_switch(regs);
}

void scheduler_init(process_t* proc) {
    // add the first process

    proc->state = PROCESS_STATE_ACTIVE;
    current_process = proc;
    current_process->id = 1;

    // note that we do not switch page directory
    // because kernel page directory is preloaded

    process_switched = true;
}

// semaphores interract closely to the scheduler so i put them here

semaphore_t* semaphore_create(unsigned max_count) {
    semaphore_t* ret = (semaphore_t*)kmalloc(sizeof(semaphore_t));

    if(ret) {
        ret->max_count = max_count;
        ret->current_count = 0;
        ret->waiting_queue.top = NULL;
        ret->waiting_queue.bottom = NULL;
        ret->waiting_queue.size = 0;
    }

    return ret;
}

void semaphore_acquire(semaphore_t* semaphore, regs_t* regs) {
    if(semaphore->current_count < semaphore->max_count) {
        semaphore->current_count++;
    }
    else {
        current_process->state = PROCESS_STATE_BLOCK;
        process_queue_push(&semaphore->waiting_queue, current_process);

        // save registers, dont add process back to ready queue
        to_next_process(regs, false);
        if(process_switched) context_switch(regs);
    }
}

void semaphore_release(semaphore_t* semaphore) {
    if(semaphore->waiting_queue.size) {
        process_t* proc = process_queue_pop(&semaphore->waiting_queue);
        proc->state = PROCESS_STATE_READY;
        process_queue_push(&ready_queue, proc);
    }
    else semaphore->current_count--;
}
