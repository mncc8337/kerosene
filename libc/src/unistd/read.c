#include <unistd.h>
#include <sys/syscall.h>

int read(int file_descriptor, void* buffer, size_t size) {
    return syscall_read(file_descriptor, buffer, size);
}
