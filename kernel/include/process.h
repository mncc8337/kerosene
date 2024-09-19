#pragma once

#include "mem.h"
#include "system.h"

#include "stdint.h"

#define MAX_PROCESSES 256

// how many ticks a thread will run before got switch to others
#define THREAD_ALIVE_TICKS 32

// TODO: use other name than PROCESS_*
#define PROCESS_STATE_SLEEP  0
#define PROCESS_STATE_ACTIVE 1

typedef struct thread {
    // all threads will have equally priority
    int id;
    int state;
    uint32_t alive_ticks;
    uint32_t stack_addr;
    regs_t regs;
    struct thread* next;
    struct thread* prev;
} thread_t;

typedef struct process {
    int id;
    int priority;
    int state;
    page_directory_t* page_directory;
    uint32_t thread_count;
    thread_t* thread_list;
    thread_t* current_thread;
    struct process* next;
    struct process* prev;
} process_t;

process_t* process_new(uint32_t eip, int priority, bool is_user);
void process_delete(process_t* proc);
thread_t* process_add_thread(process_t* proc, uint32_t eip);
void process_delete_thread(process_t* proc, thread_t* thread);

// scheduler.c
process_t* scheduler_get_process_list();
process_t* scheduler_get_current_process();
void scheduler_add_process(process_t* proc);
void scheduler_kill_process();
void scheduler_kill_thread();
void scheduler_switch(regs_t* regs);
