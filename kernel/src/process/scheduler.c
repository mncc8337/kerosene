#include "process.h"

#include "string.h"

static process_queue_t ready_queue = {NULL, NULL, 0};
static process_queue_t sleep_queue = {NULL, NULL, 0};
static process_t* current_process = NULL;

static bool process_switched = false;

static void context_switch(regs_t* regs) {
    memcpy(regs, &current_process->regs, sizeof(regs_t));
    vmmngr_switch_page_directory(current_process->page_directory);

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

// kill current process
void scheduler_kill_process(regs_t* regs) {
    // prevent killing the kernel process
    if(current_process->id == 1) return;

    process_t* saved = current_process;
    // dont save registers, dont add process back to ready queue
    to_next_process(NULL, false);

    if(process_switched) {
        process_delete(saved);
        context_switch(regs);
    }
}

void scheduler_set_sleep(regs_t* regs, unsigned ticks) {
    // set sleep target
    current_process->sleep_ticks = ticks;

    current_process->state = PROCESS_STATE_SLEEP;
    process_queue_push(&sleep_queue, current_process);
    // save registers, dont add process back to ready queue
    to_next_process(regs, false);

    if(process_switched) context_switch(regs);
}

void scheduler_switch(regs_t* regs) {
    unsigned sleep_cnt = sleep_queue.size;
    process_t* sleep_proc = process_queue_pop(&sleep_queue);
    while(sleep_cnt--) {
        sleep_proc->sleep_ticks--;
        if(sleep_proc->sleep_ticks) process_queue_push(&sleep_queue, sleep_proc);
        else {
            sleep_proc->state = PROCESS_STATE_READY;
            process_queue_push(&ready_queue, sleep_proc);
        }

        sleep_proc = process_queue_pop(&sleep_queue);
    }
    if(sleep_proc) process_queue_push(&sleep_queue, sleep_proc);

    // switch to other thread if exceeded max runtime
    if(!process_switched && current_process->alive_ticks % PROCESS_ALIVE_TICKS == 0)
        to_next_process(regs, true);

    current_process->alive_ticks++;

    if(process_switched) context_switch(regs);
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
