#include <sys/syscall.h>

void syscall_kill_process(int exit_code) {
    int ret;
    SYSCALL_1P(SYSCALL_KILL_PROCESS, ret, exit_code);
}

