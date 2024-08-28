#include "syscall.h"
#include "system.h"
#include "stdio.h"

void* _syscalls[MAX_SYSCALL] = {
    sys_syscall_test,
    sys_putchar
};

void sys_syscall_test() {
    printf("its a system call\n");
}
void sys_putchar() {
    printf("this is the putchar syscall\n");
}

void syscall_dispatcher(regs_t* regs) {
    if(regs->eax >= MAX_SYSCALL) return;

    void* fn = _syscalls[regs->eax];

    // asm volatile("call *%0" : : "r" (fn));
    int ret;
    asm volatile (
        "push %1;"
        "push %2;"
        "push %3;"
        "push %4;"
        "push %5;"
        "call *%6;"
        "pop %%ebx;"
        "pop %%ebx;"
        "pop %%ebx;"
        "pop %%ebx;"
        "pop %%ebx;"
        : "=a" (ret)
        : "r" (regs->edi), "r" (regs->esi), "r" (regs->edx), "r" (regs->ecx), "r" (regs->ebx), "r" (fn));
    regs->eax = ret;
}

void syscall_init() {
    isr_new_interrupt(0x80, 0x60, syscall_dispatcher);
}
