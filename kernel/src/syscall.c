#include "syscall.h"
#include "system.h"
#include "stdio.h"
#include "time.h"

static void syscall_test() {
    puts("this is a syscall!");
}

static void syscall_putchar(char chr) {
    putchar(chr);
}

static unsigned syscall_time() {
    return time(NULL);
}

static void* syscalls[MAX_SYSCALL] = {
    syscall_test,
    syscall_putchar,
    syscall_time
};

static void syscall_dispatcher(regs_t* regs) {
    if(regs->eax >= MAX_SYSCALL) return;

    void* fn = syscalls[regs->eax];

    // eax = func(ebx, ecx, edx, esi, edi)

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
        : "r" (regs->edi), "r" (regs->esi), "r" (regs->edx), "r" (regs->ecx), "r" (regs->ebx), "r" (fn)
    );
    regs->eax = ret;
}

void syscall_init() {
    isr_new_interrupt(0x80, syscall_dispatcher, 0xee);
}
