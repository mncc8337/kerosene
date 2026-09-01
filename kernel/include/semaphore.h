#pragma once

#include <process.h>
#include <spinlock.h>
#include <stdint.h>

typedef struct semaphore {
    spinlock_t lock;
    uint32_t count;
    process_queue_t waiting_queue;
} semaphore_t;

void semaphore_init(semaphore_t* sem, uint32_t initial_count);
semaphore_t* semaphore_create(uint32_t initial_count);
uint32_t semaphore_acquire(const regs_t* regs, semaphore_t* semaphore, uint32_t count);
void semaphore_release(semaphore_t* semaphore, uint32_t count);
int semaphore_syscall_create(const char* name, uint32_t initial_count);
uint32_t semaphore_syscall_acquire(regs_t* regs, int fd, uint32_t count);
int semaphore_syscall_release(int fd, uint32_t count);
uint32_t semaphore_syscall_kacquire(const regs_t* regs, semaphore_t* semaphore, uint32_t count);
