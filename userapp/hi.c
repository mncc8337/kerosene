#include "syscall.h"

void main() {
    int ret;
    SYSCALL_0P(SYSCALL_TEST, ret);

    SYSCALL_1P(SYSCALL_PUTCHAR, ret, 'h');
    SYSCALL_1P(SYSCALL_PUTCHAR, ret, 'e');
    SYSCALL_1P(SYSCALL_PUTCHAR, ret, 'l');
    SYSCALL_1P(SYSCALL_PUTCHAR, ret, 'l');
    SYSCALL_1P(SYSCALL_PUTCHAR, ret, 'o');
    SYSCALL_1P(SYSCALL_PUTCHAR, ret, ' ');
    SYSCALL_1P(SYSCALL_PUTCHAR, ret, 'u');
    SYSCALL_1P(SYSCALL_PUTCHAR, ret, 's');
    SYSCALL_1P(SYSCALL_PUTCHAR, ret, 'e');
    SYSCALL_1P(SYSCALL_PUTCHAR, ret, 'r');
    SYSCALL_1P(SYSCALL_PUTCHAR, ret, '!');
    SYSCALL_1P(SYSCALL_PUTCHAR, ret, '\n');

//
    return;
}
