#pragma once

#include <mem.h>
#include <system.h>
#include <filesystem.h>

#include <stdint.h>

// how many ticks a process will run before got switch to others
#define PROCESS_ALIVE_TICKS 4

#define DEFAULT_EFLAGS 0x202
#define DEFAULT_STACK_SIZE 16 * 1024

enum PROCESS_STATE {
    PROCESS_STATE_READY,
    PROCESS_STATE_ACTIVE,
    PROCESS_STATE_SLEEP,
    PROCESS_STATE_BLOCK,
};

typedef struct process {
    int id;
    int state;
    uint64_t alive_ticks;
    uint64_t sleep_ticks;
    page_directory_t* page_directory;
    uint32_t stack_addr;
    uint32_t saved_esp;
    
    file_description_t* file_descriptor_table;
    unsigned file_count;
    fs_node_t* cwd;

    struct process* next;
} process_t;

typedef struct {
    process_t* top;
    process_t* bottom;
    uint32_t size;
} process_queue_t;

#define PROCESS_QUEUE_INIT {NULL, NULL, 0}

// process.c
process_t* process_new(uint32_t eip, bool is_user);
process_t* process_make_idle();
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
uint32_t scheduler_kill_process(regs_t* regs);
uint32_t scheduler_set_sleep(regs_t* regs, unsigned ticks);
uint32_t scheduler_switch(regs_t* regs);
void scheduler_init(process_t* proc);
