#include <sys/syscall.h>

void syscall_unlock(int file_descriptor) {
    int ret;
    SYSCALL_1P(SYSCALL_UNLOCK, ret, file_descriptor);
}
