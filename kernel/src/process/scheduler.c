#include "process.h"

#include "string.h"
#include "stdio.h"
#include "system.h"

static process_t* current_process = 0;
static process_t* process_list;

static bool delete_flag = false;

static bool to_next_process(regs_t* regs) {
    // if it is the only process
    if(current_process->next == NULL && current_process->prev == NULL) return false;

    // save registers
    memcpy(&current_process->current_thread->regs, regs, sizeof(regs_t));

    // TODO: accounting for priority
    current_process->state = PROCESS_STATE_SLEEP;
    if(current_process->next) current_process = current_process->next;
    else current_process = process_list;
    current_process->state = PROCESS_STATE_ACTIVE;

    vmmngr_switch_page_directory(current_process->page_directory);
    return true;
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

    memcpy(regs, &current_process->current_thread->regs, sizeof(regs_t));
}

void scheduler_add_process(process_t* proc) {
    if(!process_list) {
        // add the first process
        process_list = proc;
        proc->next = NULL;
        proc->prev = NULL;
        return;
    }

    // add the new process to the start of the list
    proc->next = process_list;
    proc->prev = NULL;
    process_list->prev = proc;
    process_list = proc;
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

    if(!current_process) {
        current_process = process_list;
        current_process->state = PROCESS_STATE_ACTIVE;
        current_process->current_thread = current_process->thread_list;

        vmmngr_switch_page_directory(current_process->page_directory);
        memcpy(regs, &current_process->current_thread->regs, sizeof(regs_t));
        return;
    }

    bool process_changed = false;

    // switch to other process if exceeded max runtime
    if(current_process->alive_ticks % PROCESS_ALIVE_TICKS == 0) {
        process_t* saved = current_process;
        process_changed |= to_next_process(regs);

        if(delete_flag) {
            process_delete(saved);
            delete_flag = false;

            if(!process_changed) {
                // reset all
                current_process = 0;
                process_list = 0;
                return;
            }
        }
    }
    current_process->alive_ticks++;

    if(!process_changed) to_next_thread(regs);
    else memcpy(regs, &current_process->current_thread->regs, sizeof(regs_t));
}
