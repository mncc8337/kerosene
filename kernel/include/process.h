#pragma once

#include "mem.h"
#include "system.h"

#include "stdint.h"

// how many ticks a process will run before got switch to others
#define PROCESS_ALIVE_TICKS 4

enum PROCESS_STATE {
    PROCESS_STATE_READY,
    PROCESS_STATE_ACTIVE,
    PROCESS_STATE_SLEEP,
};

typedef struct process {
    int id;
    int priority;
    int state;
    uint32_t alive_ticks;
    page_directory_t* page_directory;
    uint32_t stack_addr;
    regs_t regs;
    struct process* next;
} process_t;

typedef struct {
    process_t* top;
    process_t* bottom;
} process_queue_t;

// process.c
process_t* process_new(uint32_t eip, int priority, bool is_user);
void process_delete(process_t* proc);

// process_queue.c
void process_queue_push(process_queue_t* procqueue, process_t* proc);
process_t* process_queue_pop(process_queue_t* procqueue);
bool process_queue_empty(process_queue_t* procqueue);

// scheduler.c
process_t* scheduler_get_current_process();
process_t* scheduler_get_ready_processes();
void scheduler_add_process(process_t* proc);
void scheduler_kill_process();
void scheduler_switch(regs_t* regs);
void scheduler_init(process_t* proc);
