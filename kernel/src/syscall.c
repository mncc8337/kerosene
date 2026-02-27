#include <syscall.h>
#include <system.h>
#include <process.h>
#include <filesystem.h>

#include <time.h>

#define ADD_SYSCALL(id, func) \
syscalls[id] = func

typedef int (*syscall_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
typedef uint32_t (*syscall_ctx_t)(regs_t*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

static void* syscalls[MAX_SYSCALL];

static uint32_t syscall_dispatcher(regs_t* regs) {
    // eax holds the syscall id
    // while the other hold the args
    if(regs->eax >= MAX_SYSCALL) return (uint32_t)regs;

    void* fn = syscalls[regs->eax];
    if(!fn) return (uint32_t)regs;

    bool context_switcher = (
        regs->eax == SYSCALL_KILL_PROCESS ||
        regs->eax == SYSCALL_SLEEP
    );

    if(!context_switcher) {
        int ret = ((syscall_t)fn)(regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
        regs->eax = ret;
        return (uint32_t)regs;
    }

    // handle context switchers diffently
    // since they take regs ptr as an argument
    return ((syscall_ctx_t)fn)(regs, regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
}

void syscall_init() {
    ADD_SYSCALL(SYSCALL_TIME, time);

    ADD_SYSCALL(SYSCALL_KILL_PROCESS, scheduler_kill_process);
    ADD_SYSCALL(SYSCALL_SLEEP, scheduler_set_sleep);

    ADD_SYSCALL(SYSCALL_OPEN, vfs_open);
    ADD_SYSCALL(SYSCALL_CLOSE, vfs_close);
    ADD_SYSCALL(SYSCALL_READ, vfs_read);
    ADD_SYSCALL(SYSCALL_WRITE, vfs_write);

    isr_new_interrupt(0x80, syscall_dispatcher, 0xee);
}
