#include <unistd.h>
#include <sys/syscall.h>

void close(int file_descriptor) {
    syscall_close(file_descriptor);
}
