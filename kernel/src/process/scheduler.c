#include "process.h"

#include "string.h"

static process_queue_t ready_queue = {NULL, NULL};
static process_t* current_process = NULL;

static bool process_switched = false;

static void to_next_process(regs_t* regs, bool add_back) {
    if(process_queue_empty(&ready_queue)) return;

    // save registers
    if(regs) memcpy(&current_process->regs, regs, sizeof(regs_t));
    
    current_process->state = PROCESS_STATE_READY;

    if(add_back) process_queue_push(&ready_queue, current_process);

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

void scheduler_add_process(process_t* proc) {
    proc->state = PROCESS_STATE_READY;
    process_queue_push(&ready_queue, proc);
}

// kill current process
void scheduler_kill_process() {
    // prevent killing the kernel process
    if(current_process->id == 1) return;

    process_t* saved = current_process;
    // dont save registers and dont add the process back to the queue
    to_next_process(NULL, false);
    if(process_switched) process_delete(saved);
}

void scheduler_switch(regs_t* regs) {
    // switch to other thread if exceeded max runtime
    if(!process_switched && current_process->alive_ticks % PROCESS_ALIVE_TICKS == 0)
        to_next_process(regs, true);

    current_process->alive_ticks++;

    if(process_switched) {
        // load saved registers
        memcpy(regs, &current_process->regs, sizeof(regs_t));
        vmmngr_switch_page_directory(current_process->page_directory);
    }

    process_switched = false;
}

void scheduler_init(process_t* proc) {
    // add the first process
    // this must be the kernel process

    proc->id = 1;
    proc->state = PROCESS_STATE_ACTIVE;
    current_process = proc;

    // note that we do not switch page directory
    // because kernel page directory is preloaded

    process_switched = true;
}
