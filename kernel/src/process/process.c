#include "process.h"
#include "mem.h"

#include "string.h"

static process_t* current_process;
static process_t* process_list;

int process_new(page_directory_t* pd, int priority, uint32_t eip) {
    process_t* new_process = (process_t*)kmalloc(sizeof(process_t));
    new_process->pid = current_process->pid + 1;
    new_process->priority = priority;
    new_process->page_directory = pd;
    new_process->state = PROCESS_SLEEP;
    new_process->thread_list = (thread_t*)kmalloc(sizeof(thread_t));
    new_process->next = 0;

    thread_t* first_thread = new_process->thread_list;
    first_thread->parent = current_process;
    first_thread->stack_size = 16 * 1024;
    first_thread->stack = kmalloc(first_thread->stack_size) - first_thread->stack_size;
    first_thread->kernel_stack = 0;
    first_thread->priority = 0;
    first_thread->state = PROCESS_ACTIVE;
    memset((void*)(&first_thread->frame), 0, sizeof(first_thread->frame));
    first_thread->frame.eip = eip;
    first_thread->next = 0;

    current_process->next = new_process;
    current_process = new_process;

    return new_process->pid;
}

int process_fork() {}

void process_exec(int pid) {
    process_t* proc = process_list;
    while(proc && proc ->pid != pid)
        proc = proc->next;
    if(!proc || !proc->page_directory) return;

    proc->state = PROCESS_ACTIVE;

    thread_t* first_thread = proc->thread_list;

    page_directory_t* saved_pd = vmmngr_get_directory();
    uint32_t saved_esp; asm volatile("mov %%esp, %%eax" : "=a" (saved_esp));
    vmmngr_switch_page_directory(proc->page_directory);

    // asm("mov %0, %%esp" : : "r"(first_thread->stack));
    asm("mov %0, %%eax; call %%eax" : : "r"(first_thread->frame.eip));

    vmmngr_switch_page_directory(saved_pd);
    asm("mov %0, %%esp" : : "r"(saved_esp));
}

void process_switch() {}

void process_init() {
    // default process (kernel process)
    current_process = (process_t*)kmalloc(sizeof(process_t));
    current_process->pid = 0;
    current_process->priority = 0;
    current_process->page_directory = vmmngr_get_directory();
    current_process->state = PROCESS_ACTIVE;
    current_process->thread_list = (thread_t*)kmalloc(sizeof(thread_t));
    current_process->next = 0;

    thread_t* first_thread = current_process->thread_list;
    first_thread->parent = current_process;
    // first_thread->stack_size = 16 * 1024;
    // first_thread->stack = kmalloc(first_thread->stack_size) - first_thread->stack_size;
    first_thread->kernel_stack = 0;
    first_thread->priority = 0;
    first_thread->state = PROCESS_ACTIVE;
    memset((void*)(&first_thread->frame), 0, sizeof(first_thread->frame));
    first_thread->next = 0;

    process_list = current_process;
}
