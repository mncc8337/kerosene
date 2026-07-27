#include <sys/syscall.h>

void syscall_yield() {
    int ret;
    SYSCALL_0P(SYSCALL_YIELD, ret);
}
