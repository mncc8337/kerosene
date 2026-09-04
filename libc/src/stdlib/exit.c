#include <sys/syscall.h>

__attribute__((__noreturn__))
void exit(int exit_code) {
    syscall_kill(exit_code);
    while(1) asm volatile ("pause"); 
}
