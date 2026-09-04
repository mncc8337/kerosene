#include <sys/syscall.h>

void syscall_attach(void* process_addr) {
    int ret;
    SYSCALL_1P(SYSCALL_DETACH, ret, process_addr);
}
