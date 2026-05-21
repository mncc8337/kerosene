#include <unistd.h>
#include <sys/syscall.h>

void close(int file_descriptor) {
    return syscall_close(file_descriptor);
}
