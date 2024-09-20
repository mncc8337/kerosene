#include "process.h"

#include "string.h"
#include "stdio.h"
#include "system.h"

static process_t* current_process = NULL;
static process_t* process_list_tail = NULL;

static bool process_switched = false;

static void to_next_process(regs_t* regs, bool add_back) {
    if(current_process == process_list_tail) return;

    // save registers
    if(regs) memcpy(&current_process->regs, regs, sizeof(regs_t));
    
    // scheduler_add_process will set current_process-next to NULL
    // so we need to save it
    process_t* next_proc = current_process->next;
    if(add_back) scheduler_add_process(current_process);

    current_process->state = PROCESS_STATE_READY;
    current_process = next_proc;
    current_process->state = PROCESS_STATE_ACTIVE;

    process_switched = true;
}

process_t* scheduler_get_current_process() {
    return current_process;
}

void scheduler_add_process(process_t* proc) {
    if(!current_process) {
        // add the first process
        // this must be the kernel process
        process_list_tail = proc;
        current_process = proc;
        current_process->state = PROCESS_STATE_READY;
        current_process->next = NULL;
        process_switched = true;
        // note that we do not switch page directory
        // because kernel page directory is preloaded
        return;
    }

    proc->next = NULL;
    process_list_tail->next = proc;
    process_list_tail = proc;
}

// kill current process
void scheduler_kill_process() {
    // prevent killing the kernel process
    // this also ensure that the process queue always contains at least 2 elements
    if(current_process->id == 1) return;

    process_t* saved = current_process;
    // dont save registers and dont add the process back to the queue
    to_next_process(NULL, false);
    // process_switched should be set because there are at least 2 elements on the queue
    process_delete(saved);
}

void scheduler_switch(regs_t* regs) {
    // switch to other thread if exceeded max runtime
    if(!process_switched && current_process->alive_ticks % PROCESS_ALIVE_TICKS == 0)
        to_next_process(regs, true);

    current_process->alive_ticks++;

    if(process_switched) {
        memcpy(regs, &current_process->regs, sizeof(regs_t));
        vmmngr_switch_page_directory(current_process->page_directory);
    }

    process_switched = false;
}
