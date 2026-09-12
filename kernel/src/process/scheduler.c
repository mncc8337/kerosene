#include <process.h>
#include <kutils.h>
#include <system.h>
#include <misc/elf.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/files.h>

static process_queue_t ready_queue = PROCESS_QUEUE_INIT;
// this is a linked list sorted by sleep_ticks
// TODO: use a priority queue instead
static process_queue_t sleep_queue = PROCESS_QUEUE_INIT;
static process_queue_t delete_queue = PROCESS_QUEUE_INIT;

static process_t* current_process = NULL;

static process_t* global_list_bottom = NULL;
static size_t global_list_size = 0;

static uint64_t global_sleep_ticks = 0;

static void global_list_push(process_t* proc) {
    global_list_bottom->global_next = proc;
    proc->global_prev = global_list_bottom;
    proc->global_next = NULL;
    global_list_bottom = proc;
    global_list_size++;
}

static void global_list_pop(process_t* proc) {
    // assumption: since the idle process is the first process added to the list
    // then proc->global_prev always exists
    proc->global_prev->global_next = proc->global_next;
    if(proc->global_next) {
        proc->global_next->global_prev = proc->global_prev;
    }

    if(proc == global_list_bottom) {
        global_list_bottom = proc->global_prev;
    }
    global_list_size--;
}

process_t* scheduler_get_current() {
    return current_process;
}

void scheduler_push_ready(process_t* proc) {
    proc->state = PROCESS_STATE_READY;
    process_queue_push(&ready_queue, proc);
}

void scheduler_add_process(process_t* proc) {
    proc->state = PROCESS_STATE_READY;
    global_list_push(proc);
    process_queue_push(&ready_queue, proc);
}

uint32_t scheduler_to_next_process(const regs_t* regs, bool add_back) {
    // save regs before switching
    current_process->saved_esp = (uint32_t)regs;

    if(add_back)
        scheduler_push_ready(current_process);

    current_process = process_queue_pop(&ready_queue);
    current_process->state = PROCESS_STATE_ACTIVE;

    vmmngr_switch_page_directory(current_process->page_directory);
    tss_set_stack(current_process->tss_esp0);
    return current_process->saved_esp;
}

// this function remove the current process from the scheduler,
// store it to the attached process and only push it back to the
// scheduler when the attached process dies
uint32_t scheduler_attach(const regs_t* regs, process_t* proc) {
    // NOTE: proc must be a newly created process and not added to the ready queue before

    // should only be called by the kernel
    if((regs->cs & 0x03) != 0) {
        return scheduler_kill_process(regs, 13);
        return (uint32_t)regs;
    }

    proc->attached_from = current_process;
    scheduler_add_process(proc);
    return scheduler_to_next_process(regs, false);
}

int scheduler_spawn(
    const char* path,
    bool is_user,
    unsigned argc,
    char* args,
    bool attach,
    fs_node_t* stdin,
    fs_node_t* stdout,
    int* returned_value
) {
    process_t* current = current_process;
    if(current->is_user && !is_user)
        return -1;

    if(current->is_user && !validate_user_buffer(current, args, ARGS_MAX_LEN))
        return -1;

    page_directory_t* pd = NULL;
    if(is_user) {
        pd = vmmngr_alloc_page_directory();
        if(!pd)
            return -1;
    }

    uint32_t eip;
    ELF_ERR load_err = elf_load((char*)path, pd, &eip);

    if(load_err) {
        // FS_ERR ferr = elf_get_err();
        if(is_user) vmmngr_free_page_directory(pd);
        return -1;
    }

    process_t* proc = process_new(eip, is_user, pd, NULL, argc, args);
    if(!proc) {
        if(is_user) vmmngr_free_page_directory(pd);
        return -1;
    }

    file_description_t* caller_fdt = current_process->file_descriptor_table;
    if(!stdin) {
        stdin = caller_fdt[SYSFILE_FD_STDIN].node;
        if(!stdin)
            stdin = vfs_get_stdin();
    }

    if(!stdout) {
        stdout = caller_fdt[SYSFILE_FD_STDOUT].node;
        if(!stdout)
            stdout = vfs_get_stdout();
    }

    process_set_stdfile(
        proc,
        stdin,
        FILE_OPEN_READ,
        stdout,
        FILE_OPEN_WRITE | FILE_OPEN_APPEND
    );

    if(attach) {
        syscall_attach(proc);
        if(returned_value)
            *returned_value = current->received_exit_code;
        return 0;
    }

    scheduler_add_process(proc);
    return 0;
}

int scheduler_syscall_spawn(syscall_spawn_args_t* args) {
    fs_node_t* stdin_node = NULL;
    fs_node_t* stdout_node = NULL;

    process_t* current = current_process;

    if(args->stdin_path) {
        FS_ERR err = vfs_find_and_create_node(
            args->stdin_path,
            current->cwd,
            &stdin_node,
            FILE_OPEN_READ,
            0
        );
        if(err) return -1;
    }

    if(args->stdout_path) {
        FS_ERR err = vfs_find_and_create_node(
            args->stdout_path,
            current->cwd,
            &stdout_node,
            FILE_OPEN_WRITE | FILE_OPEN_APPEND,
            0
        );
        if(err) return -1;
    }

    int ret;
    int result = scheduler_spawn(
        args->path,
        args->is_user,
        args->argc,
        args->args,
        args->attach,
        stdin_node,
        stdout_node,
        &ret
    );
    if(result >= 0)
        return ret;
    return result;
}

// put current process to delete queue, delete it later
uint32_t scheduler_kill_process(const regs_t* regs, int exit_code) {
    if(current_process->id == 1) return (uint32_t)regs; // avoid deleting idle process

    if(current_process->attached_from) {
        // push the original process back to the queue
        current_process->attached_from->received_exit_code = exit_code;
        scheduler_push_ready(current_process->attached_from);
    }

    // because we are using the stack of the process
    // and process_delete() will free the stack
    // we will postpone the process deleting
    // and do it later when we are using other process's stack
    process_queue_push(&delete_queue, current_process);

    // dont add process back to ready queue
    return scheduler_to_next_process(regs, false);
}

uint32_t scheduler_set_sleep(const regs_t* regs, unsigned ticks) {
    // set sleep target
    current_process->sleep_ticks = ticks + global_sleep_ticks;

    current_process->state = PROCESS_STATE_SLEEP;
    process_queue_sorted_push(&sleep_queue, current_process, process_sort_by_sleep_ticks);

    return scheduler_to_next_process(regs, false);
}

uint32_t scheduler_switch(const regs_t* regs) {
    
    // FIXME:
    // dont delete process on the spot
    // it is more ideal to have a dedicated process to clean up those zombies
    // because upon deleting process it need to save opended files
    // accessing files on an interrupts requests blocks system progress

    while(delete_queue.size) {
        process_t* proc = process_queue_pop(&delete_queue);
        global_list_pop(proc);
        process_delete(proc);
    }

    if(sleep_queue.size) {
        global_sleep_ticks++;
        while(sleep_queue.top && sleep_queue.top->sleep_ticks <= global_sleep_ticks) {
            process_t* proc = process_queue_pop(&sleep_queue);
            scheduler_push_ready(proc);
        }

        // avoid overflow
        // i mean this solution is suck because if you sleep() for a very long time
        // then global_sleep_ticks will overflow, causing all process in the queue to wake up
        // or did not sleep at all
        // separate ticks into hours/minutes/seconds structure maybe the solution
        if(sleep_queue.size == 0) global_sleep_ticks = 0;
    }

    current_process->alive_ticks++;
    current_process->saved_esp = (uint32_t)regs;

    // switch to other thread if exceeded max runtime
    if(current_process->alive_ticks % PROCESS_ALIVE_TICKS == 0)
        return scheduler_to_next_process(regs, true);

    return current_process->saved_esp;
}

void scheduler_init(process_t* idle_proc) {
    // add the first process

    idle_proc->state = PROCESS_STATE_ACTIVE;
    current_process = idle_proc;

    idle_proc->global_next = NULL;
    idle_proc->global_prev = NULL;

    // setting up the global process list
    global_list_bottom = idle_proc;
    global_list_size = 1;

    // note that we do not switch page directory
    // because kernel page directory is preloaded
}
