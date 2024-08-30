#include "syscall.h"
#include "system.h"
#include "stdio.h"

void sys_syscall_test() {
    printf("its a system call\n");
}
void sys_putchar() {
    printf("this is the putchar syscall\n");
}

static void* syscalls[MAX_SYSCALL] = {
    sys_syscall_test,
    sys_putchar
};

static void syscall_dispatcher(regs_t* regs) {
    if(regs->eax >= MAX_SYSCALL) return;

    void* fn = syscalls[regs->eax];

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
    isr_new_interrupt(0x80, syscall_dispatcher, 0xee);
}
