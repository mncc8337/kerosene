#pragma once

#include <stdint.h>
#include <sys/syscall.h>

#include <mem.h>
#include <system.h>

// how many ticks a process will run before got switch to others
#define PROCESS_ALIVE_TICKS 4

// TODO: move these to .env
#define DEFAULT_EFLAGS 0x202
#define KERNEL_STACK_SIZE (8 * 1024)
#define USER_STACK_SIZE (64 * 1024)

#define ARGS_MAX_LEN 512
#define ENVS_MAX_LEN 1024

enum PROCESS_STATE {
    PROCESS_STATE_READY,
    PROCESS_STATE_ACTIVE,
    PROCESS_STATE_SLEEP,
    PROCESS_STATE_BLOCK,
};

typedef struct process {
    int id;
    int state;
    uint64_t alive_ticks;
    uint64_t sleep_ticks;
    page_directory_t* page_directory;
    bool is_user;

    uint8_t argc; // arg counter
    char args[ARGS_MAX_LEN]; // args separated by null \0, must be double null-terminated
    char envs[ENVS_MAX_LEN]; // same as args but for env vars

    int received_exit_code; // return code from started process
                            // only set if the child process is attached
    struct process* attached_from;

    uint32_t stack_addr; // stack addr used to freeing
    uint32_t saved_esp; // saved stack state of current process
    uint32_t tss_esp0; // the stack to use when handling interrupts (usr proc only)
    
    uint32_t waiting_for_resource_count; // how many resources the process is currently blocked waiting for

    struct file_description* file_descriptor_table;
    unsigned file_count;
    struct fs_node* cwd;

    struct process* queue_next; // next item in a local queue
    struct process* global_next; // next item in the global process list
    struct process* global_prev; // previous item in the global process list
} process_t;

typedef struct {
    process_t* top;
    process_t* bottom;
    uint32_t size;
} process_queue_t;

#define PROCESS_QUEUE_INIT {NULL, NULL, 0}

// process.c
process_t* process_new(uint32_t eip, bool is_user, page_directory_t* pagedir, struct fs_node* cwd, char* args, char* envs);
process_t* process_make_idle();
void process_delete(process_t* proc);
void process_set_stdfile(process_t* proc, struct fs_node* stdin, uint32_t stdin_flags, struct fs_node* stdout, uint32_t stdout_flags);

// process_queue.c
bool process_sort_by_sleep_ticks(process_t* a, process_t* b);
void process_queue_push(process_queue_t* procqueue, process_t* proc);
void process_queue_sorted_push(process_queue_t* procqueue, process_t* proc, bool (*cmp)(process_t*, process_t*));
process_t* process_queue_pop(process_queue_t* procqueue);

// scheduler.c
process_t* scheduler_get_current();
void scheduler_push_ready(process_t* proc);
void scheduler_add_process(process_t* proc);
uint32_t scheduler_to_next_process(const regs_t* regs, bool add_back);
uint32_t scheduler_attach(const regs_t* regs, process_t* proc);
int scheduler_spawn(const char* path, bool is_user, char* args, char* envs, bool attach, struct fs_node* stdin, struct fs_node* stdout, int* returned_value);
int scheduler_syscall_spawn(syscall_spawn_args_t* args);
uint32_t scheduler_kill_process(const regs_t* regs, int exit_code);
uint32_t scheduler_set_sleep(const regs_t* regs, unsigned ticks);
uint32_t scheduler_switch(const regs_t* regs);
void scheduler_init(process_t* proc);
