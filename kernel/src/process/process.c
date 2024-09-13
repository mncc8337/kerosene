#include "process.h"
#include "system.h"
#include "mem.h"

#include "string.h"

static process_t* current_process;
static process_t* process_list;
static process_t* process_list_tail;

static process_t** process_stack;
static unsigned process_stack_pos;

extern uint32_t get_eip();

static void process_stack_push(process_t* proc) {
    if(process_stack_pos < MAX_PROCESSES)
        process_stack[process_stack_pos++] = proc;
    else kernel_panic(NULL);
}

static void process_stack_pop() {
    if(process_stack_pos > 0) process_stack_pos--;
}

static process_t* process_stack_top() {
    return process_stack[process_stack_pos-1];
}

process_t* process_get_current() {
    return current_process;
}

int process_new(uint32_t eip) {
    process_t* new_process = (process_t*)kmalloc(sizeof(process_t));
    if(!new_process) return 0;
    new_process->pid = process_list_tail->pid + 1;
    new_process->page_directory = vmmngr_alloc_page_directory();
    if(!new_process->page_directory) return 0;
    new_process->state = PROCESS_STATE_SLEEP;
    new_process->thread_list = (thread_t*)kmalloc(sizeof(thread_t));
    if(!new_process->thread_list) return 0;
    new_process->next = 0;
    new_process->prev = process_list_tail;

    thread_t* first_thread = new_process->thread_list;
    first_thread->parent = new_process;
    memset((void*)(&first_thread->frame), 0, sizeof(first_thread->frame));
    first_thread->frame.eip = eip;
    first_thread->next = 0;

    process_list_tail->next = new_process;
    process_list_tail = new_process;
    return new_process->pid;
}

void process_exec(int pid) {
    // save current process state
    if(current_process) {
        int caller;
        asm volatile("pop %%eax; push %%eax" : "=a"(caller));
        current_process->thread_list->frame.eip = caller;
        // TODO: save more things
    }

    process_t* proc = process_list;
    if(current_process->pid == pid) proc = current_process;
    else {
        while(proc->pid != pid && proc->next != 0) proc = proc->next;
        if(proc->pid != pid || !proc->page_directory) return;
    }

    current_process->state = PROCESS_STATE_SLEEP;
    process_stack_push(proc);
    current_process = proc;
    current_process->state = PROCESS_STATE_ACTIVE;

    thread_t* first_thread = proc->thread_list;

    // FIXME:
    // somehow syscall with more than 1 param will cause a page fault when called
    // may be related to the stack

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

    process_t* proc = current_process;
    if(!proc || !proc->page_directory || proc->pid == 0) {
        process_stack_pop();
        return;
    }

    // load kernel page directory
    vmmngr_switch_page_directory(process_list->page_directory);

    // free page tables
    vmmngr_map_temporary_pd(proc->page_directory);
    page_directory_t* virt_pd = (page_directory_t*)VMMNGR_RESERVED;
    for(unsigned i = 0; i < 1024; i++)
        if(virt_pd->entry[i] != 0)
            pmmngr_free_block((void*)(virt_pd->entry[i] >> 12));
    // now free the page directory
    pmmngr_free_block((void*)proc->page_directory);

    kfree(proc->thread_list);
    // join process list
    if(proc->prev) proc->prev->next = proc->next;
    kfree(proc);

    process_stack_pop();
    current_process = process_stack_top();
    // TODO: somehow run this process
}

void process_switch() {}

bool process_init() {
    // default process (kernel process)
    current_process = (process_t*)kmalloc(sizeof(process_t));
    if(!current_process) return true;
    current_process->pid = 1;
    current_process->page_directory = vmmngr_get_directory();
    current_process->state = PROCESS_STATE_ACTIVE;
    current_process->thread_list = (thread_t*)kmalloc(sizeof(thread_t));
    if(!current_process->thread_list) return true;
    current_process->next = 0;
    current_process->prev = 0;

    thread_t* first_thread = current_process->thread_list;
    first_thread->parent = current_process;

    extern char kernel_stack_bottom;
    extern char kernel_stack_top;
    first_thread->stack_size = &kernel_stack_top - &kernel_stack_bottom;
    void* esp; asm volatile("mov %%esp, %%eax" : "=a"(esp));
    first_thread->stack = esp;
    memset((void*)(&first_thread->frame), 0, sizeof(first_thread->frame));
    first_thread->next = 0;

    process_list = current_process;
    process_list_tail = current_process;

    process_stack = kmalloc(sizeof(process_t*) * MAX_PROCESSES);
    if(!process_stack) return true;

    process_stack_push(current_process);

    return false;
}
