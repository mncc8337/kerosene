#include <sys/syscall.h>

void syscall_semaphore_release(int fd) {
    int ret;
    SYSCALL_1P(SYSCALL_SEMAPHORE_RELEASE, ret, fd);
}

