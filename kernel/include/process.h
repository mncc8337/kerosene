#pragma once

#include "mem.h"

#define PROC_SLEEP  0
#define PROC_ACTIVE 1

typedef struct process {
    int pid;
    int priority;
    page_directory_t* page_directory;
    int state;
    struct thread* thread_list;
    struct process* next;
} process_t;

// typedef struct {
//     uint32_t esp;
//     uint32_t ebp;
//     uint32_t eip;
//     uint32_t edi;
//     uint32_t esi;
//     uint32_t eax;
//     uint32_t ebx;
//     uint32_t ecx;
//     uint32_t edx;
//     uint32_t flags;
// } trap_frame_t;

typedef struct thread {
    process_t* parent;
    void* stack;
    void* stack_max;
    void* kernel_stack;
    uint32_t priority;
    int state;
    // trap_frame_t frame;
} thread_t;

void process_fork();
void process_exec();
void process_switch();
void process_init();
