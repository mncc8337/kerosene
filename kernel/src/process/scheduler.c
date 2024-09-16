#include "process.h"

#include "string.h"

static process_t* current_process = 0;
static thread_t* current_thread;
static process_t* process_list;

static bool delete_flag = false;

static bool to_next_process() {
    // if it is the only process
    if(current_process->next == NULL && current_process->prev == NULL) return false;

    // TODO: accounting for priority
    current_process->state = PROCESS_STATE_SLEEP;
    if(current_process->next) current_process = current_process->next;
    else current_process = process_list;
    current_process->state = PROCESS_STATE_ACTIVE;
    current_thread = current_process->thread_list;
    return true;
}

void scheduler_add_process(process_t* proc) {
    if(!process_list) {
        // add the first process
        process_list = proc;
        proc->next = NULL;
        proc->prev = NULL;
        return;
    }

    process_t* prev_proc = process_list;

    // add the process in decrease priority order
    while(prev_proc->next != NULL && prev_proc->next->priority <= proc->priority)
        prev_proc = prev_proc->next;

    proc->next = prev_proc->next;
    proc->prev = prev_proc;
    prev_proc->next = proc;
}

void scheduler_terminate_process() {
    // remove current process from the list
    if(current_process->prev)
        current_process->prev->next = current_process->next;
    if(current_process->next)
        current_process->next->prev = current_process->prev;

    // set alive ticks to 0
    // to force switch process as soon as the timer fires another tick
    current_process->alive_ticks = 0;

    delete_flag = true;
}

void scheduler_switch(regs_t* regs) {
    if(!process_list) return;

    bool process_changed = false;
    bool thread_changed = false;

    if(!current_process) {
        current_process = process_list;
        current_process->state = PROCESS_STATE_ACTIVE;

        current_thread = current_process->thread_list;

        process_changed = true;
        thread_changed = true;
        goto apply_changes;
    }

    // switch to other process if exceeded max runtime
    if(current_process->alive_ticks % PROCESS_ALIVE_TICKS == 0) {
        bool proc_switched;
        if(delete_flag) {
            process_t* saved = current_process;
            proc_switched = to_next_process();
            process_delete(saved);
            delete_flag = false;
        }
        else {
            // increase so that it does not got switched on next tick
            current_process->alive_ticks++;
            proc_switched = to_next_process();
        }

        if(proc_switched) {
            process_changed = true;
            thread_changed = true;
        }
    }
    else current_process->alive_ticks++;

    // switch to next thread
    // TODO: accounting for priority
    if(current_thread->next || current_thread != current_process->thread_list) {
        // save registers
        memcpy(&current_thread->regs, regs, sizeof(regs_t));
        current_thread->state = PROCESS_STATE_SLEEP;

        if(current_thread->next) current_thread = current_thread->next;
        else current_thread = current_process->thread_list;
        current_thread->state = PROCESS_STATE_ACTIVE;

        thread_changed = true;
    }

    apply_changes:
    if(process_changed) {
        // load new page directory
        vmmngr_switch_page_directory(current_process->page_directory);
    }

    if(thread_changed) {
        // load saved registers
        memcpy(regs, &current_thread->regs, sizeof(regs_t));
    }
}
