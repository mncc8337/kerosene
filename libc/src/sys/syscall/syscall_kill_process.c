#include <sys/syscall.h>

void syscall_kill_process() {
    int ret;
    SYSCALL_0P(SYSCALL_KILL_PROCESS, ret);
}

