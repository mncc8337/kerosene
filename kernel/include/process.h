#pragma once

#include "mem.h"
#include "system.h"
#include "filesystem.h"

#include "stdint.h"

// how many ticks a process will run before got switch to others
#define PROCESS_ALIVE_TICKS 4

enum PROCESS_STATE {
    PROCESS_STATE_READY,
    PROCESS_STATE_ACTIVE,
    PROCESS_STATE_SLEEP,
    PROCESS_STATE_BLOCK,
};

typedef struct process {
    int id;
    int priority;
    int state;
    uint64_t alive_ticks;
    uint64_t sleep_ticks;
    page_directory_t* page_directory;
    uint32_t stack_addr;
    regs_t regs;
    
    file_description_t* file_descriptor_table;
    unsigned* file_count;
    fs_node_t* cwd;

    struct process* next;
} process_t;

typedef struct {
    process_t* top;
    process_t* bottom;
    uint32_t size;
} process_queue_t;

#define PROCESS_QUEUE_INIT {NULL, NULL, 0}

typedef struct {
    uint32_t max_count;
    uint32_t current_count;
    process_queue_t waiting_queue;
} semaphore_t;

// process.c
process_t* process_new(uint32_t eip, int priority, bool is_user, bool is_thread);
void process_delete(process_t* proc);

// process_queue.c
bool process_sort_by_sleep_ticks(process_t* a, process_t* b);
void process_queue_push(process_queue_t* procqueue, process_t* proc);
void process_queue_sorted_push(process_queue_t* procqueue, process_t* proc, bool (*cmp)(process_t*, process_t*));
process_t* process_queue_pop(process_queue_t* procqueue);

// scheduler.c
process_t* scheduler_get_current_process();
process_t* scheduler_get_ready_processes();
process_t* scheduler_get_sleep_processes();
void scheduler_add_process(process_t* proc);
void scheduler_kill_process(regs_t* regs);
void scheduler_set_sleep(regs_t* regs, unsigned ticks);
void scheduler_switch(regs_t* regs);
void scheduler_init(process_t* proc);

// scheduler.c
semaphore_t* semaphore_create(unsigned max_count);
void semaphore_acquire(semaphore_t* semaphore, regs_t* regs);
void semaphore_release(semaphore_t* semaphore);
