#include <sys/syscall.h>

void syscall_semaphore_release(void* addr) {
    int ret;
    SYSCALL_1P(SYSCALL_SEMAPHORE_RELEASE, ret, addr);
}

