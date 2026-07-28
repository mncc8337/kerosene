#include <sys/syscall.h>

void syscall_semaphore_acquire(int fd) {
    int ret;
    SYSCALL_1P(SYSCALL_SEMAPHORE_ACQUIRE, ret, fd);
}
