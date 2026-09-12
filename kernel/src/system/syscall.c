#include <system.h>
#include <sys/syscall.h>
#include <process.h>
#include <filesystem.h>
#include <timer.h>
#include <semaphore.h>
#include <kutils.h>

typedef int (*syscall_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
typedef uint32_t (*syscall_ctx_t)(const regs_t*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

static void* syscalls[MAX_SYSCALL] = {
    [SYSCALL_TIME] = &timer_syscall_get_current_time,

    [SYSCALL_SEMAPHORE_CREATE] = &semaphore_syscall_create,
    [SYSCALL_SEMAPHORE_ACQUIRE] = &semaphore_syscall_acquire,
    [SYSCALL_SEMAPHORE_RELEASE] = &semaphore_syscall_release,
    [SYSCALL_SEMAPHORE_KACQUIRE] = &semaphore_syscall_kacquire,

    [SYSCALL_YIELD] = &scheduler_to_next_process,
    [SYSCALL_DETACH] = &scheduler_attach,
    [SYSCALL_SPAWN] = &scheduler_syscall_spawn,
    [SYSCALL_KILL] = &scheduler_kill_process,
    [SYSCALL_SLEEP] = &scheduler_set_sleep,

    [SYSCALL_OPEN] = &vfs_open,
    [SYSCALL_CLOSE] = &vfs_close,
    [SYSCALL_LOCK] = &vfs_lock,
    [SYSCALL_UNLOCK] = &vfs_unlock,
    [SYSCALL_READ] = &vfs_read,
    [SYSCALL_WRITE] = &vfs_write,
    [SYSCALL_SEEK] = &vfs_seek,
    [SYSCALL_MOUNT] = &vfs_mount,
};

static bool context_switchers[MAX_SYSCALL] = {
    [SYSCALL_SEMAPHORE_ACQUIRE] = 1,
    [SYSCALL_SEMAPHORE_KACQUIRE] = 1,

    [SYSCALL_YIELD] = 1,
    [SYSCALL_DETACH] = 1,
    [SYSCALL_KILL] = 1,
    [SYSCALL_SLEEP] = 1,

    [SYSCALL_LOCK] = 1,
};

static uint32_t syscall_dispatcher(regs_t* regs) {
    // eax holds the syscall id
    // while the other hold the args
    if(regs->eax >= MAX_SYSCALL) return (uint32_t)regs;

    void* fn = syscalls[regs->eax];
    if(!fn) return (uint32_t)regs;

    if(!context_switchers[regs->eax]) {
        int ret = ((syscall_t)fn)(regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
        regs->eax = ret;
        return (uint32_t)regs;
    }

    // handle context switchers diffently
    // since they take regs ptr as the first argument
    // NOTE: context switchers set their return value by setting regs->eax
    return ((syscall_ctx_t)fn)(regs, regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
}

void syscall_init() {
    isr_new_interrupt(0x80, syscall_dispatcher, 0xee);
}
