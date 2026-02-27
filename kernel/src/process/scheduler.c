#include <process.h>
#include <system.h>

static process_queue_t ready_queue = PROCESS_QUEUE_INIT;
// this is a linked list sorted by sleep_ticks
// TODO: use a priority queue instead
static process_queue_t sleep_queue = PROCESS_QUEUE_INIT;
static process_queue_t delete_queue = PROCESS_QUEUE_INIT;

static process_t* current_process = NULL;

static uint64_t global_sleep_ticks = 0;

static bool to_next_process(regs_t* regs, bool add_back) {
    if(!ready_queue.size) return false;

    // save regs before switching
    current_process->saved_esp = (uint32_t)regs;

    if(add_back) {
        current_process->state = PROCESS_STATE_READY;
        process_queue_push(&ready_queue, current_process);
    }

    current_process = process_queue_pop(&ready_queue);
    current_process->state = PROCESS_STATE_ACTIVE;

    vmmngr_switch_page_directory(current_process->page_directory);
    tss_set_stack(current_process->tss_esp0);

    return true;
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
uint32_t scheduler_kill_process(regs_t* regs) {
    if(current_process->id == 1) return (uint32_t)regs; // avoid deleting idle process

    // because we are using the stack of the process
    // and process_delete() will free the stack
    // we will postpone the process deleting
    // and do it later when we are using other process's stack
    process_queue_push(&delete_queue, current_process);

    // dont add process back to ready queue
    to_next_process(regs, false);

    return current_process->saved_esp;
}

uint32_t scheduler_set_sleep(regs_t* regs, unsigned ticks) {
    // set sleep target
    current_process->sleep_ticks = ticks + global_sleep_ticks;

    current_process->state = PROCESS_STATE_SLEEP;
    process_queue_sorted_push(&sleep_queue, current_process, process_sort_by_sleep_ticks);

    to_next_process(regs, false);

    return current_process->saved_esp;
}

uint32_t scheduler_switch(regs_t* regs) {
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

    current_process->alive_ticks++;
    current_process->saved_esp = (uint32_t)regs;

    // switch to other thread if exceeded max runtime
    if(current_process->alive_ticks % PROCESS_ALIVE_TICKS == 0)
        to_next_process(regs, true);

    return current_process->saved_esp;
}

void scheduler_init(process_t* idle_proc) {
    // add the first process

    idle_proc->state = PROCESS_STATE_ACTIVE;
    current_process = idle_proc;

    // note that we do not switch page directory
    // because kernel page directory is preloaded
}
