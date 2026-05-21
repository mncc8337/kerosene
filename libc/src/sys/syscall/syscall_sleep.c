#include <sys/syscall.h>

void syscall_sleep(unsigned ticks) {
    int ret;
    SYSCALL_1P(SYSCALL_SLEEP, ret, ticks);
}

