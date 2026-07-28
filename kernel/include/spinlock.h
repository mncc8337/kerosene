#pragma once

#include <stdatomic.h>

#define SPINLOCK_INIT ATOMIC_FLAG_INIT
#define spinlock_init(lock) atomic_flag_clear(lock)

typedef volatile atomic_flag spinlock_t;

void spinlock_acquire(spinlock_t* lock);
void spinlock_release(spinlock_t* lock);
