#include <unistd.h>
#include <sys/syscall.h>

int write(int file_descriptor, const void* buffer, size_t size) {
    return syscall_write(file_descriptor, (const uint8_t*)buffer, size);
}
