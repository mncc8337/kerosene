#include <sys/syscall.h>

int syscall_semaphore_create(const char* name, uint32_t max_count) {
    int ret;
    SYSCALL_2P(SYSCALL_SEMAPHORE_CREATE, ret, name, max_count);
    return ret;
}
