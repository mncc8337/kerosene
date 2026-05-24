#include <stdint.h>
#include <sys/syscall.h>

uint64_t syscall_time() {
    int dummy_ret;
    uint64_t ret = 0;
    SYSCALL_1P(SYSCALL_TIME, dummy_ret, &ret);
    return ret;
}
