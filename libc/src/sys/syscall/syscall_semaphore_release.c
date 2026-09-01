#include <sys/syscall.h>

void syscall_semaphore_release(int fd, uint32_t count) {
    int ret;
    SYSCALL_2P(SYSCALL_SEMAPHORE_RELEASE, ret, fd, count);
}

