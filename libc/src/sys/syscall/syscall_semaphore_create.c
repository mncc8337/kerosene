#include <sys/syscall.h>

int syscall_semaphore_create(const char* name, uint32_t initial_count) {
    int ret;
    SYSCALL_2P(SYSCALL_SEMAPHORE_CREATE, ret, name, initial_count);
    return ret;
}
