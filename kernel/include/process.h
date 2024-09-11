#pragma once

#include "stdint.h"
#include "mem.h"

#define PROCESS_SLEEP  0
#define PROCESS_ACTIVE 1

typedef struct thread {
    struct process* parent;
    void* stack;
    uint32_t stack_size;
    void* kernel_stack;
    uint32_t priority;
    int state;
    struct {
        uint32_t esp;
        uint32_t ebp;
        uint32_t eip;
        uint32_t edi;
        uint32_t esi;
        uint32_t eax;
        uint32_t ebx;
        uint32_t ecx;
        uint32_t edx;
        uint32_t flags;
    } frame;
    struct thread* next;
} thread_t;

typedef struct process {
    int pid;
    int priority;
    page_directory_t* page_directory;
    int state;
    thread_t* thread_list;
    struct process* next;
} process_t;

int process_new(page_directory_t* pd, int priority, uint32_t eip);
int process_fork();
void process_exec();
void process_terminate();
void process_switch();
void process_init();
