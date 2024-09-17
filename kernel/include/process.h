#pragma once

#include "mem.h"
#include "system.h"

#include "stdint.h"

#define MAX_PROCESSES 256

// how many ticks a process will run before got switch to others
#define PROCESS_ALIVE_TICKS 32

#define PROCESS_STATE_SLEEP  0
#define PROCESS_STATE_ACTIVE 1

typedef struct thread {
    int id;
    int priority;
    int state;
    regs_t regs;
    struct thread* next;
    struct thread* prev;
} thread_t;

typedef struct process {
    int id;
    int priority;
    int state;
    uint32_t alive_ticks;
    page_directory_t* page_directory;
    uint32_t thread_count;
    thread_t* thread_list;
    thread_t* current_thread;
    struct process* next;
    struct process* prev;
} process_t;

process_t* process_new(uint32_t eip, int priority, bool is_user);
void process_delete(process_t* proc);
bool process_add_thread(process_t* proc, uint32_t eip, int priority);

// scheduler.c
process_t* scheduler_get_process_list();
process_t* scheduler_get_current_process();
void scheduler_add_process(process_t* proc);
void scheduler_terminate_process();
void scheduler_switch(regs_t* regs);
