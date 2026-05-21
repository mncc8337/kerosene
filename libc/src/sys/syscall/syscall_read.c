#include <sys/syscall.h>

int syscall_read(int file_descriptor, uint8_t* buffer, size_t size) {
    int ret;
    SYSCALL_3P(SYSCALL_READ, ret, file_descriptor, buffer, size);
    return ret;
}
