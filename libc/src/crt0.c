#include <sys/syscall.h>

extern int main();

void _start() {
    int exit_code = main();
    syscall_kill(exit_code);
    while(1) asm volatile ("pause");
}
