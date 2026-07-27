#pragma once

#include <stdatomic.h>

void spinlock_acquire(volatile atomic_flag* lock);
void spinlock_release(volatile atomic_flag* lock);
