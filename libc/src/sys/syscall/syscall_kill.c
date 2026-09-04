#include <sys/syscall.h>

void syscall_kill(int exit_code) {
    int ret;
    SYSCALL_1P(SYSCALL_KILL, ret, exit_code);
}

