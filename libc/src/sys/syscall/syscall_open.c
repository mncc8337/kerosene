#include <sys/syscall.h>

int syscall_open(const char* path, const char* modestr) {
    int ret;
    SYSCALL_2P(SYSCALL_OPEN, ret, path, modestr);
    return ret;
}
