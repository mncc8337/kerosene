#include "process.h"

#include "string.h"
#include "stdio.h"

static process_t* current_process = 0;
static process_t* process_list;

// should be turned on after switch to another process
static bool process_switched = false;

static void to_next_process(regs_t* regs) {
    // if it is the only process
    if(current_process->next == NULL && current_process->prev == NULL) return;

    if(regs) memcpy(&current_process->current_thread->regs, regs, sizeof(regs_t));

    // TODO: accounting for priority
    current_process->state = PROCESS_STATE_SLEEP;
    if(current_process->next) current_process = current_process->next;
    else current_process = process_list;
    current_process->state = PROCESS_STATE_ACTIVE;

    process_switched = true;
}

static void to_next_thread(regs_t* regs) {
    if(current_process->thread_count == 1) return;
    // TODO: accounting for priority

    thread_t* current_thread = current_process->current_thread;

    // save registers
    memcpy(&current_thread->regs, regs, sizeof(regs_t));
    current_thread->state = PROCESS_STATE_SLEEP;

    if(current_thread->next)
        current_thread = current_thread->next;
    else current_thread = current_process->thread_list;
    current_thread->state = PROCESS_STATE_ACTIVE;
    current_process->current_thread = current_thread;

    memcpy(regs, &current_thread->regs, sizeof(regs_t));
}

process_t* scheduler_get_process_list() {
    return process_list;
}

process_t* scheduler_get_current_process() {
    return current_process;
}

void scheduler_add_process(process_t* proc) {
    if(!process_list) {
        // add the first process
        // this must be the kernel process
        proc->next = NULL;
        proc->prev = NULL;
        process_list = proc;
        current_process = proc;
        current_process->state = PROCESS_STATE_ACTIVE;
        current_process->current_thread = current_process->thread_list;
        process_switched = true;
        // note that we do not switch page directory
        // because kernel page directory is preloaded
        return;
    }

    // add the new process right after the first process
    proc->next = process_list->next;
    proc->prev = process_list;
    process_list->next = proc;
}

// terminate current process
void scheduler_terminate_process() {
    // do not terminate the first process
    if(current_process == process_list) return;

    // remove current process from the list
    if(current_process->prev)
        current_process->prev->next = current_process->next;
    if(current_process->next)
        current_process->next->prev = current_process->prev;

    process_t* saved = current_process;
    // we will delete the process afterward so there is no need to save registers
    to_next_process(NULL);
    // process_switched flag will not turn on if we only have 1 process left
    // in that case do not delete the process
    if(process_switched) process_delete(saved);
}

void scheduler_switch(regs_t* regs) {
    // switch to other process if exceeded max runtime
    if(current_process->alive_ticks % PROCESS_ALIVE_TICKS == 0)
        to_next_process(regs);

    current_process->alive_ticks++;

    if(!process_switched) to_next_thread(regs);
    else {
        vmmngr_switch_page_directory(current_process->page_directory);
        memcpy(regs, &current_process->current_thread->regs, sizeof(regs_t));
    }

    process_switched = false;
}
