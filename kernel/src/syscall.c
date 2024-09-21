#include "syscall.h"
#include "system.h"
#include "process.h"

#include "stdio.h"
#include "time.h"

#define ADD_SYSCALL(id, func) \
syscalls[id] = func

static void* syscalls[MAX_SYSCALL];

static regs_t* current_regs;

static void test_syscall() {
    puts("this is a syscall!");
}

static void proc_kill() {
    scheduler_kill_process(current_regs);
}

static void proc_sleep(unsigned ticks) {
    scheduler_set_sleep(current_regs, ticks);
}

static void syscall_dispatcher(regs_t* regs) {
    if(regs->eax >= MAX_SYSCALL) return;

    void* fn = syscalls[regs->eax];
    if(!fn) return;

    current_regs = regs;

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
    ADD_SYSCALL(SYSCALL_TEST, test_syscall);
    ADD_SYSCALL(SYSCALL_PUTCHAR, putchar);
    ADD_SYSCALL(SYSCALL_TIME, time);
    ADD_SYSCALL(SYSCALL_KILL_PROCESS, proc_kill);
    ADD_SYSCALL(SYSCALL_SLEEP, proc_sleep);

    isr_new_interrupt(0x80, syscall_dispatcher, 0xee);
}
