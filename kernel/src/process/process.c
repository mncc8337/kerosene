#include "process.h"
#include "system.h"
#include "mem.h"

#include "kshell.h"

#include "string.h"
#include "stdio.h"

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

void process_exec() {
    // process_t* proc = process_list;
    // while(proc && proc->pid != pid)
    //     proc = proc->next;
    process_t* proc = current_process;
    if(!proc || !proc->page_directory) return;

    thread_t* first_thread = proc->thread_list;

    uint32_t esp = 0;
    asm("mov %%esp, %%eax" : "=a" (esp));
    tss_set_stack(esp);

    vmmngr_switch_page_directory(proc->page_directory);

    // create a heap for the process
    heap_t* heap = heap_new(UHEAP_START, UHEAP_INITIAL_SIZE, UHEAP_MAX_SIZE, 0b00);
    first_thread->stack_size = 16 * 1024;
    first_thread->stack = heap_alloc(heap, first_thread->stack_size, false) - first_thread->stack_size;

    // enter user mode
    asm(
        "cli;"
        "mov $0x23, %%ax;"
        "mov %%ax, %%ds;"
        "mov %%ax, %%es;"
        "mov %%ax, %%fs;"
        "mov %%ax, %%gs;"

        "pushl $0x23;"
        "pushl %0;" // esp
        "pushf;"

        "pop %%eax;"
        "or $0x200, %%eax;"
        "push %%eax;"

        "pushl $0x1b;"
        "push %1;" // eip
        "iret;"
        : : "r" (first_thread->stack), "r" (first_thread->frame.eip)
    );
}

void process_terminate() {
    // NOTE: WIP!

    process_t* proc = current_process;
    if(!proc || !proc->page_directory || proc->pid == 0) return;

    // get back to kernel mode
    asm(
        "cli;"
        "mov $0x10, %eax;"
        "mov %ax, %ds;"
        "mov %ax, %es;"
        "mov %ax, %fs;"
        "mov %ax, %gs;"
        "sti;"
    );

    // free page tables
    vmmngr_map_temporary_pd(proc->page_directory);
    page_directory_t* virt_pd = (page_directory_t*)VMMNGR_RESERVED;
    for(unsigned i = 0; i < 1024; i++)
        if(virt_pd->entry[i] != 0)
            pmmngr_free_block((void*)(virt_pd->entry[i] >> 12));
    // now free the page directory
    pmmngr_free_block((void*)proc->page_directory);

    // store the pid and free memory
    int pid = proc->pid;
    kfree((void*)proc);

    // find the previous process and set it to current process
    process_t* prev_proc = 0;
    proc = process_list;
    while(proc && proc->pid != pid) {
        prev_proc = proc;
        proc = proc->next;
    }
    if(prev_proc == 0) return; // this will never happend
    prev_proc->next = 0;
    vmmngr_switch_page_directory(prev_proc->page_directory);
    current_process = prev_proc;

    // comeback to kshell
    // TODO: it is better to continue the previous process lol
    shell_start();
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
