#pragma once

#include <process.h>
#include <spinlock.h>

typedef struct semaphore {
    spinlock_t lock;
    uint32_t current_count;
    uint32_t max_count;
    process_queue_t waiting_queue;
} semaphore_t;

semaphore_t* semaphore_create(uint32_t max_count);
uint32_t semaphore_acquire(regs_t* regs, semaphore_t* semaphore);
void semaphore_release(semaphore_t* semaphore);
int semaphore_syscall_create(const char* name, uint32_t max_count);
uint32_t semaphore_syscall_acquire(regs_t* regs, int fd);
int semaphore_syscall_release(int fd);
