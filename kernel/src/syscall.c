#include "syscall.h"
#include "system.h"
#include "process.h"
#include "filesystem.h"

#include "stdio.h"
#include "time.h"
#include "string.h"

#define ADD_SYSCALL(id, func) \
syscalls[id] = func

static void* syscalls[MAX_SYSCALL];

static regs_t regs_copy;

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
    // somehow modifying the registers directly will very likely to cause a page fault
    // so we make a copy of it
    // and only copy it back when err_code is 1 (turned on when loading saved registers)
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

    // make a volatile ptr copy so that compiler will not optimize it out
    volatile regs_t* vol_regs_copy = &regs_copy;

    // compiler will optimize out the code below if we dont use the volatile ptr defined above
    // because it will think regs_copy will not change, which is true for it because it does not know
    // what the inline asm would do to regs_copy and assume that regs_copy will not change

    // if regs_copy.err_code changed (only when context is switched)
    // then load regs_copy to the original regs pointer
    // and DO NOT CHANGE eax (returned value) because of context switching
    // make sure that there are no context switching function that also return a value
    // since it is not support here
    if(vol_regs_copy->err_code) memcpy(regs, &regs_copy, sizeof(regs_t));
    else regs->eax = ret;
}

void syscall_init() {
    ADD_SYSCALL(SYSCALL_PUTCHAR, putchar);
    ADD_SYSCALL(SYSCALL_TIME, time);

    ADD_SYSCALL(SYSCALL_KILL_PROCESS, proc_kill);
    ADD_SYSCALL(SYSCALL_SLEEP, proc_sleep);

    ADD_SYSCALL(SYSCALL_OPEN, vfs_open);
    ADD_SYSCALL(SYSCALL_CLOSE, vfs_close);
    ADD_SYSCALL(SYSCALL_READ, vfs_read);
    ADD_SYSCALL(SYSCALL_WRITE, vfs_write);

    isr_new_interrupt(0x80, syscall_dispatcher, 0xee);
}
