#include "process.h"
#include "system.h"
#include "mem.h"

#include "string.h"
#include "stdio.h"

#define KERNEL_PROC_PID 1

#define DEFAULT_EFLAGS 0x202

static process_t* current_process = 0;
static process_t* process_list;
static process_t* process_list_tail;

static process_t** process_stack;
static unsigned process_stack_pos;

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

process_t* process_new(uint32_t eip, bool is_user) {
    if(!current_process) return 0;

    process_t* new_process = (process_t*)kmalloc(sizeof(process_t));
    if(!new_process) return 0;
    new_process->pid = process_list_tail->pid + 1;
    new_process->page_directory = vmmngr_alloc_page_directory();
    if(!new_process->page_directory) return 0;
    new_process->state = PROCESS_STATE_SLEEP;
    new_process->is_user = is_user;
    new_process->thread_list = (thread_t*)kmalloc(sizeof(thread_t));
    if(!new_process->thread_list) return 0;
    new_process->next = 0;
    new_process->prev = process_list_tail;

    thread_t* first_thread = new_process->thread_list;
    first_thread->parent = new_process;
    memset((void*)(&first_thread->frame), 0, sizeof(first_thread->frame));

    page_directory_t* saved_pd = vmmngr_get_directory();
    vmmngr_switch_page_directory(new_process->page_directory);

    heap_t* heap = heap_new(UHEAP_START, UHEAP_INITIAL_SIZE, UHEAP_MAX_SIZE, 0b00);
    first_thread->stack_size = 16 * 1024;
    first_thread->stack = heap_alloc(heap, first_thread->stack_size, false) + first_thread->stack_size;
    first_thread->frame.esp = (uint32_t)first_thread->stack;

    vmmngr_switch_page_directory(saved_pd);

    first_thread->frame.eflags = DEFAULT_EFLAGS;
    first_thread->frame.eip = eip;
    first_thread->next = 0;

    process_list_tail->next = new_process;
    process_list_tail = new_process;
    return new_process;
}

void process_switch(process_t* proc) {
    if(!proc) return;

    if(current_process) {
        // save current process state
        // TODO: do this for all threads

        thread_t* fthread = current_process->thread_list;
        stackframe_t* stk;
        asm volatile(
            // "mov %%eax, %0;"
            // "mov %%ebx, %1;"
            // "mov %%ecx, %2;"
            // "mov %%edx, %3;"
            // "mov %%esi, %4;"
            // "mov %%edi, %5;"
            "mov %%esp, %6;"
            "mov %%ebp, %7;"
            :
            "=g" (fthread->frame.eax), "=g" (fthread->frame.ebx), "=g" (fthread->frame.ecx),
            "=g" (fthread->frame.edx), "=g" (fthread->frame.esi), "=g" (fthread->frame.edi),
            "=g" (fthread->frame.esp), "=g" (stk)
        );

        // fix stack
        fthread->frame.esp += 64;

        fthread->frame.ebp = (uint32_t)stk->ebp;
        fthread->frame.eip = (uint32_t)stk->eip;

        if(current_process->pid == KERNEL_PROC_PID)
            tss_set_stack(fthread->frame.esp);

        current_process->state = PROCESS_STATE_SLEEP;
    }
    else {
        // if current process is zero
        // that mean the switch call is from the interrupts handler
        // in that case we have to fix stack
        // because we have do pusha, push ds, ... (60 bytes)
        // but we havent pop them yet
        // also we do not return control to the interrupts handler
        // so that those value are still in the stack
        // also trash value pushed by the interrupts hander (4*3 bytes)
        // because we dont need them
        asm volatile(
            "mov %esp, %eax;"
            "add $72, %eax;"
            "mov %eax, %esp;"
        );
    }

    vmmngr_switch_page_directory(proc->page_directory);

    thread_t* fthread = proc->thread_list;

    proc->state = PROCESS_STATE_ACTIVE;
    process_stack_push(proc);
    current_process = proc;

    // FIXME: load eax, ebx, .. do not work
    if(proc->is_user) {
        asm volatile(
            // enter user mode
            "mov $0x23, %%ax;"
            "mov %%ax, %%ds;"
            "mov %%ax, %%es;"
            "mov %%ax, %%fs;"
            "mov %%ax, %%gs;"

            "push $0x23;"
            "push %0;" // esp

            "push %1;" // eflags

            "push $0x1b;"
            "push %2;" // eip

            // load saved state
            // TODO: somehow do this for every threads
            // "mov %3, %%eax;"
            // "mov %4, %%ebx;"
            // "mov %5, %%ecx;"
            // "mov %6, %%edx;"
            // "mov %7, %%esi;"
            // "mov %8, %%edi;"
            "mov %9, %%ebp;"

            // let's go
            "iret;"
            : :
            "g" (fthread->frame.esp), "g" (fthread->frame.eflags), "g" (fthread->frame.eip),
            "g" (fthread->frame.eax), "g" (fthread->frame.ebx), "g" (fthread->frame.ecx),
            "g" (fthread->frame.edx), "g" (fthread->frame.esi), "g" (fthread->frame.edi),
            "g" (fthread->frame.ebp)
        );
    }
    else {
        asm volatile(
            // we have already in kernel mode

            // load saved state
            // TODO: somehow do this for every threads
            // "mov %0, %%eax;"
            // "mov %1, %%ebx;"
            // "mov %2, %%ecx;"
            // "mov %3, %%edx;"
            // "mov %4, %%esi;"
            // "mov %5, %%edi;"
            "mov %6, %%ebp;"
            // "mov %7, %%esp;" // something is wrong with this, uncomment this will crash

            "jmp *%8;"
            : :
            "g" (fthread->frame.eax), "g" (fthread->frame.ebx), "g" (fthread->frame.ecx),
            "g" (fthread->frame.edx), "g" (fthread->frame.esi), "g" (fthread->frame.edi),
            "g" (fthread->frame.ebp), "g" (fthread->frame.esp), "g" (fthread->frame.eip)
        );
    }
}

void process_terminate() {
    // NOTE:
    // this function is intended to be called upon an interrupt
    // please do not call it as a normal function

    process_t* proc = current_process;
    if(!proc || !proc->page_directory || proc->pid == KERNEL_PROC_PID) {
        if(proc->pid != KERNEL_PROC_PID) process_stack_pop();
        return;
    }

    // free the heap before switch to kernel page directory
    heap_t* heap = (heap_t*)UHEAP_START;
    for(virtual_addr_t i = (virtual_addr_t)heap; i < heap->end; i += MMNGR_PAGE_SIZE)
        pmmngr_free_block((void*)vmmngr_to_physical_addr(NULL, i));

    // load kernel page directory
    vmmngr_switch_page_directory(process_list->page_directory);

    // free page tables
    vmmngr_map_temporary_pd(proc->page_directory);
    page_directory_t* virt_pd = (page_directory_t*)VMMNGR_RESERVED;
    for(unsigned i = 0; i < 1024; i++)
        if(virt_pd->entry[i] != 0) pmmngr_free_block((void*)(virt_pd->entry[i] >> 12));
    // now free the page directory
    pmmngr_free_block((void*)proc->page_directory);

    // free all threads
    thread_t* thread = proc->thread_list;
    while(thread) {
        thread_t* tmp = thread->next;
        kfree(thread);
        thread = tmp;
    }

    // join process list
    if(proc->prev) proc->prev->next = proc->next;
    kfree(proc);

    // resume last process
    process_stack_pop();
    current_process = 0;
    process_switch(process_stack_top());
}

bool process_init() {
    // default process (kernel process)
    current_process = (process_t*)kmalloc(sizeof(process_t));
    if(!current_process) return true;
    current_process->pid = KERNEL_PROC_PID;
    current_process->page_directory = vmmngr_get_directory();
    current_process->state = PROCESS_STATE_ACTIVE;
    current_process->is_user = false;
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
    first_thread->frame.eflags = DEFAULT_EFLAGS;
    first_thread->next = 0;

    process_list = current_process;
    process_list_tail = current_process;

    process_stack = kmalloc(sizeof(process_t*) * MAX_PROCESSES);
    if(!process_stack) return true;

    process_stack_push(current_process);

    return false;
}
