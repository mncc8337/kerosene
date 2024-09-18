#include "process.h"

#include "string.h"

static process_t* current_process = 0;
static process_t* process_list;

static bool process_switched = false;

static process_t* get_next_process() {
    // TODO: accounting for priority
    if(current_process->next) return current_process->next;
    return process_list;
}

static void to_next_process(regs_t* regs) {
    // if it is the only process
    if(current_process->next == NULL && current_process->prev == NULL) return;

    if(regs) memcpy(&current_process->current_thread->regs, regs, sizeof(regs_t));

    current_process->state = PROCESS_STATE_SLEEP;
    current_process = get_next_process();
    current_process->state = PROCESS_STATE_ACTIVE;

    process_switched = true;
}

static thread_t* get_next_thread() {
    // TODO: accounting for priority
    if(current_process->current_thread->next)
        return current_process->current_thread->next;
    return current_process->thread_list;
}

static void to_next_thread(regs_t* regs) {
    if(current_process->thread_count < 2) return;

    thread_t* current_thread = current_process->current_thread;
    thread_t* next_thread = get_next_thread();

    bool thread_del = current_thread->state == PROCESS_STATE_DEAD;
    while(current_thread->state == PROCESS_STATE_DEAD) {
        process_delete_thread(current_process, current_thread);
        current_thread = next_thread;
        next_thread = get_next_thread();
    }
    if(!thread_del) {
        // save registers
        memcpy(&current_thread->regs, regs, sizeof(regs_t));
        current_thread->state = PROCESS_STATE_SLEEP;
    }

    current_thread = next_thread;
    current_thread->state = PROCESS_STATE_ACTIVE;
    current_process->current_thread = current_thread;

    // load saved registers
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

// kill current process
void scheduler_kill_process() {
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

void scheduler_kill_thread() {
    // mark current thread as dead
    // it will be deleted upon process switch or thread switch
    current_process->current_thread->state = PROCESS_STATE_DEAD;
}

void scheduler_switch(regs_t* regs) {
    // recently discovered that this scheduler is called round robin

    // switch to other process if exceeded max runtime
    if(current_process->alive_ticks % PROCESS_ALIVE_TICKS == 0) to_next_process(regs);
    else if(current_process->thread_count == 0) scheduler_kill_process();

    current_process->alive_ticks++;

    if(!process_switched)
        to_next_thread(regs);
    else {
        vmmngr_switch_page_directory(current_process->page_directory);
        memcpy(regs, &current_process->current_thread->regs, sizeof(regs_t));
    }

    process_switched = false;
}