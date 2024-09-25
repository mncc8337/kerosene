#include "system.h"

void spinlock_acquire(spinlock_t* lock) {
    // TODO: check rank
    while(atomic_flag_test_and_set_explicit(&lock->flag, memory_order_acquire))
        __builtin_ia32_pause();
}

void spinlock_release(spinlock_t* lock) {
    atomic_flag_clear_explicit(&lock->flag, memory_order_release);
}
