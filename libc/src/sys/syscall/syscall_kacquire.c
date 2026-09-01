#include <sys/syscall.h>

void syscall_semaphore_kacquire(void* semaphore, uint32_t count) {
    int ret;
    SYSCALL_2P(SYSCALL_SEMAPHORE_KACQUIRE, ret, semaphore, count);
}
