#include <sys/syscall.h>

int syscall_lock(int file_descriptor) {
    int ret;
    SYSCALL_1P(SYSCALL_LOCK, ret, file_descriptor);
    return ret;
}
