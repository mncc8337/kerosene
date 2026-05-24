#include <unistd.h>
#include <sys/syscall.h>

off_t lseek(int file_descriptor, off_t offset, int whence) {
    off_t ret;
    syscall_seek(file_descriptor, offset, whence, &ret);
    return ret;
}

