#include "syscall.h"
#include "system.h"
#include "process.h"

#include "stdio.h"
#include "time.h"
#include "string.h"

#define ADD_SYSCALL(id, func) \
syscalls[id] = func

static void* syscalls[MAX_SYSCALL];

static regs_t regs_copy;

static void test_syscall() {
    puts("this is a syscall!");
}

static void proc_kill() {
    scheduler_kill_process(&regs_copy);
}

static void proc_sleep(unsigned ticks) {
    scheduler_set_sleep(&regs_copy, ticks);
}

static void syscall_dispatcher(regs_t* regs) {
    if(regs->eax >= MAX_SYSCALL) return;

    void* fn = syscalls[regs->eax];
    if(!fn) return;

    // NOTE:
    // since changing directly into the registers save will cause page fault
    // on calling SYSCALL_SLEEP (dont ask, idk why), we need to make an copy of it
    // and make changes to it (some how this do works lmao)
    // when done loading new registers we set err_code to 1
    // so that we would know when to copy the changed registers back to the original one
    // yeah this is a hack, to fix a wtf problem
    memcpy(&regs_copy, regs, sizeof(regs_t));
    regs_copy.err_code = 0;

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

    putchar(0); // idk why without this line the code below wont run

    // check if regs has changed
    if(regs_copy.err_code)
        memcpy(regs, &regs_copy, sizeof(regs_t));
}

void syscall_init() {
    ADD_SYSCALL(SYSCALL_TEST, test_syscall);
    ADD_SYSCALL(SYSCALL_PUTCHAR, putchar);
    ADD_SYSCALL(SYSCALL_TIME, time);
    ADD_SYSCALL(SYSCALL_KILL_PROCESS, proc_kill);
    ADD_SYSCALL(SYSCALL_SLEEP, proc_sleep);

    isr_new_interrupt(0x80, syscall_dispatcher, 0xee);
}
