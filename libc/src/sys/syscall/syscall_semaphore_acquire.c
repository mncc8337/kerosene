#include <sys/syscall.h>

void syscall_semaphore_acquire(void* addr) {
    int ret;
    SYSCALL_1P(SYSCALL_SEMAPHORE_ACQUIRE, ret, addr);
}
