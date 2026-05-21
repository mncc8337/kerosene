#include <sys/syscall.h>

int syscall_write(int file_descriptor, const uint8_t* buffer, size_t size) {
    int ret;
    SYSCALL_3P(SYSCALL_WRITE, ret, file_descriptor, buffer, size);
    return ret;
}
