#include "process.h"
#include "system.h"

#include "string.h"
#include "stdio.h"

static process_queue_t ready_queue = {NULL, NULL, 0};
// this is a linked list sorted by sleep_ticks
// TODO: use a priority queue instead
static process_queue_t sleep_queue = {NULL, NULL, 0};
static process_queue_t delete_queue = {NULL, NULL, 0};

static process_t* current_process = NULL;

static uint64_t global_sleep_ticks = 0;

static bool process_switched = false;

static volatile atomic_flag lock = ATOMIC_FLAG_INIT;

static void context_switch(regs_t* regs) {
    memcpy(regs, &current_process->regs, sizeof(regs_t));
    vmmngr_switch_page_directory(current_process->page_directory);

    // this regs ptr maybe a copy so set err_code to 1
    // see syscall.c syscall_dispatcher() notes
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
    spinlock_acquire(&lock);
    process_t* ret = current_process;
    spinlock_release(&lock);

    return ret;
}

process_t* scheduler_get_ready_processes() {
    spinlock_acquire(&lock);
    process_t* ret = ready_queue.top;
    spinlock_release(&lock);

    return ret;
}

process_t* scheduler_get_sleep_processes() {
    spinlock_acquire(&lock);
    process_t* ret = sleep_queue.top;
    spinlock_release(&lock);

    return ret;
}

void scheduler_add_process(process_t* proc) {
    spinlock_acquire(&lock);

    proc->state = PROCESS_STATE_READY;
    process_queue_push(&ready_queue, proc);

    spinlock_release(&lock);
}

// put current process to delete queue, delete it later
void scheduler_kill_process(regs_t* regs) {
    if(current_process->id == 1) return; // avoid deleting kernel process

    spinlock_acquire(&lock);

    // because we are using the stack of the process
    // and process_delete() will free the stack
    // we will postpone the process deleting
    // and do it later when we are using other process's stack
    process_queue_push(&delete_queue, current_process);

    // dont save registers, dont add process back to ready queue
    to_next_process(NULL, false);
    // process switching always happens because there is always a kernel process to switch to
    context_switch(regs);

    spinlock_release(&lock);
}

void scheduler_set_sleep(regs_t* regs, unsigned ticks) {
    spinlock_acquire(&lock);

    // set sleep target
    current_process->sleep_ticks = ticks + global_sleep_ticks;

    current_process->state = PROCESS_STATE_SLEEP;
    process_queue_sorted_push(&sleep_queue, current_process, process_sort_by_sleep_ticks);
    // save registers, dont add process back to ready queue
    to_next_process(regs, false);

    if(process_switched) context_switch(regs);

    spinlock_release(&lock);
}

void scheduler_switch(regs_t* regs) {
    spinlock_acquire(&lock);

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

    spinlock_release(&lock);
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
