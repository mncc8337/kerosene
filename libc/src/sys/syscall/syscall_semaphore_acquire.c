#include <sys/syscall.h>

void syscall_semaphore_acquire(int fd, uint32_t count) {
    int ret;
    SYSCALL_2P(SYSCALL_SEMAPHORE_ACQUIRE, ret, fd, count);
}
