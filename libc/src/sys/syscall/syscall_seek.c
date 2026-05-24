#include <sys/syscall.h>

void syscall_seek(
    int file_descriptor,
    int64_t offset,
    int whence,
    int64_t* position
) {
    int ret;
    SYSCALL_5P(
        SYSCALL_SEEK,
        ret,
        file_descriptor,
        (uint32_t)(offset >> 32 & 0xffffffff),
        (uint32_t)(offset & 0xffffffff),
        whence,
        position
    );
}
