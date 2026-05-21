#include <sys/syscall.h>

void syscall_close(int file_descriptor) {
    int ret;
    SYSCALL_1P(SYSCALL_CLOSE, ret, file_descriptor);
}

