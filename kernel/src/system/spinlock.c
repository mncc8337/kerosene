#include "system.h"

void spinlock_acquire(atomic_flag* lock) {
    while(atomic_flag_test_and_set_explicit(lock, memory_order_acquire))
        __builtin_ia32_pause();
}

void spinlock_release(atomic_flag* lock) {
    atomic_flag_clear_explicit(lock, memory_order_release);
}
