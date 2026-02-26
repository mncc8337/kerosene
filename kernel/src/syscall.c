#include <syscall.h>
#include <system.h>
#include <process.h>
#include <filesystem.h>

#include <time.h>
#include <string.h>

#define ADD_SYSCALL(id, func) \
syscalls[id] = func

static void* syscalls[MAX_SYSCALL];

static regs_t regs_copy;

static void sys_prockill() {
    scheduler_kill_process(&regs_copy);
}

static void sys_sleep(unsigned ticks) {
    scheduler_set_sleep(&regs_copy, ticks);
}

static void syscall_dispatcher(regs_t* regs) {
    if(regs->eax >= MAX_SYSCALL) return;

    void* fn = syscalls[regs->eax];
    if(!fn) return;

    // NOTE:
    // somehow modifying the registers directly will very likely to cause a page fault
    // so we make a copy of it
    // and only copy it back when context is switched
    memcpy(&regs_copy, regs, sizeof(regs_t));

    bool context_switched = false;
    context_switched |= regs->eax == SYSCALL_KILL_PROCESS;
    context_switched |= regs->eax == SYSCALL_SLEEP;

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

    // if context is switched
    // then load regs_copy to the original regs pointer
    // and DO NOT CHANGE eax (returned value) because of context switching
    if(context_switched) memcpy(regs, &regs_copy, sizeof(regs_t));
    else regs->eax = ret;
}

void syscall_init() {
    ADD_SYSCALL(SYSCALL_TIME, time);

    ADD_SYSCALL(SYSCALL_KILL_PROCESS, sys_prockill);
    ADD_SYSCALL(SYSCALL_SLEEP, sys_sleep);

    ADD_SYSCALL(SYSCALL_OPEN, vfs_open);
    ADD_SYSCALL(SYSCALL_CLOSE, vfs_close);
    ADD_SYSCALL(SYSCALL_READ, vfs_read);
    ADD_SYSCALL(SYSCALL_WRITE, vfs_write);

    isr_new_interrupt(0x80, syscall_dispatcher, 0xee);
}
