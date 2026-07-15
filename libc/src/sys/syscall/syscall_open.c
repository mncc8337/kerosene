#include <sys/syscall.h>

int syscall_open(const char* path, const file_mode_t mode) {
    int ret;
    SYSCALL_2P(SYSCALL_OPEN, ret, path, mode);
    return ret;
}
