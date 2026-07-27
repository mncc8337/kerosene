#pragma once

#include <process.h>

typedef struct {
    uint32_t max_count;
    uint32_t current_count;
    process_queue_t waiting_queue;
} semaphore_t;

semaphore_t* semaphore_create(unsigned max_count);
uint32_t semaphore_acquire(regs_t* regs, semaphore_t* semaphore);
void semaphore_release(semaphore_t* semaphore);
