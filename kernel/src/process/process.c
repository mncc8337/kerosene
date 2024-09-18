#include "process.h"
#include "system.h"
#include "mem.h"

#define DEFAULT_EFLAGS 0x202
#define DEFAULT_STACK_SIZE 16 * 1024

static unsigned process_count = 0;

static bool is_user_proc(process_t* proc) {
    return proc->page_directory != vmmngr_get_kernel_page_directory();
}

process_t* process_new(uint32_t eip, int priority, bool is_user) {
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    if(!proc) return NULL;

    proc->id = process_count + 1;
    proc->priority = priority;
    proc->state = PROCESS_STATE_SLEEP;
    proc->alive_ticks = 1;
    if(!is_user) proc->page_directory = vmmngr_get_kernel_page_directory();
    else {
        // only users need to have a separate page directory
        proc->page_directory = vmmngr_alloc_page_directory();
        if(!proc->page_directory) {
            kfree(proc);
            return NULL;
        }
    }
    proc->thread_count = 1;
    proc->thread_list = (thread_t*)kmalloc(sizeof(thread_t));
    if(!proc->thread_list) {
        vmmngr_free_page_directory(proc->page_directory);
        kfree(proc);
        return NULL;
    }
    proc->current_thread = proc->thread_list;
    proc->next = NULL;
    proc->prev = NULL;

    thread_t* thread = proc->thread_list;
    thread->id = 1;
    thread->priority = 0;
    thread->state = PROCESS_STATE_ACTIVE;
    thread->next = NULL;

    regs_t* regs = &thread->regs;
    regs->eip = eip;
    regs->eflags = DEFAULT_EFLAGS;
    if(is_user) {
        // switch page directory to create user heap
        vmmngr_switch_page_directory(proc->page_directory);

        heap_t* heap = heap_new(UHEAP_START, UHEAP_INITIAL_SIZE, UHEAP_MAX_SIZE, 0b00);
        if(!heap) {
            vmmngr_switch_page_directory(vmmngr_get_kernel_page_directory());
            kfree(proc->thread_list);
            vmmngr_free_page_directory(proc->page_directory);
            kfree(proc);
            return NULL;
        }

        regs->cs = 0x1b; // user code selector
        regs->ds = 0x23; // user data selector
        regs->es = regs->ds;
        regs->fs = regs->ds;
        regs->gs = regs->ds;
        regs->ss = regs->ds;
        // this should yield no error because the heap is new
        thread->stack_addr = (uint32_t)heap_alloc(heap, DEFAULT_STACK_SIZE, false);

        vmmngr_switch_page_directory(vmmngr_get_kernel_page_directory());
    }
    else {
        regs->cs = 0x08; // kernel code selector
        regs->ds = 0x10; // kernel data selector
        regs->es = regs->ds;
        regs->fs = regs->ds;
        regs->gs = regs->ds;
        regs->ss = regs->ds;
        thread->stack_addr = (uint32_t)kmalloc(DEFAULT_STACK_SIZE);

        if(!thread->stack_addr) {
            kfree(proc->thread_list);
            kfree(proc);
            return NULL;
        }
    }
    regs->useresp = thread->stack_addr + DEFAULT_STACK_SIZE;

    process_count++;

    return proc;
}

void process_delete(process_t* proc) {
    // all kernel process use the same page directory so do not touch it
    if(is_user_proc(proc)) vmmngr_free_page_directory(proc->page_directory);

    // free all threads
    thread_t* thread = proc->thread_list;
    while(thread) {
        void* t = thread;
        thread = thread->next;
        kfree(t);
    }

    kfree(proc);
}

thread_t* process_add_thread(process_t* proc, uint32_t eip, int priority) {
    thread_t* new = (thread_t*)kmalloc(sizeof(thread_t));
    if(!new) return NULL;

    unsigned new_id = 1;

    // TODO: maybe store the final thread?
    thread_t* final_thread = proc->thread_list;
    if(final_thread) {
        while(final_thread->next)
            final_thread = final_thread->next;
        new_id = final_thread->id + 1;
    }

    new->id = new_id;
    new->priority = priority;
    new->state = PROCESS_STATE_SLEEP;
    new->next = NULL;
    new->prev = NULL;

    regs_t* regs = &new->regs;
    regs->eip = eip;
    regs->eflags = DEFAULT_EFLAGS;
    if(is_user_proc(proc)) {
        vmmngr_switch_page_directory(proc->page_directory);
        regs->cs = 0x1b; // user code selector
        regs->ds = 0x23; // user data selector
        regs->es = regs->ds;
        regs->fs = regs->ds;
        regs->gs = regs->ds;
        regs->ss = regs->ds;
        new->stack_addr = (uint32_t)heap_alloc((heap_t*)UHEAP_START, DEFAULT_STACK_SIZE, false);
        vmmngr_switch_page_directory(vmmngr_get_kernel_page_directory());
    }
    else {
        regs->cs = 0x08; // kernel code selector
        regs->ds = 0x10; // kernel data selector
        regs->es = regs->ds;
        regs->fs = regs->ds;
        regs->gs = regs->ds;
        regs->ss = regs->ds;
        new->stack_addr = (uint32_t)kmalloc(DEFAULT_STACK_SIZE);
    }
    if(!new->stack_addr) {
        kfree(new);
        return NULL;
    }
    regs->useresp = new->stack_addr + DEFAULT_STACK_SIZE;

    if(final_thread) {
        final_thread->next = new;
        new->prev = final_thread;
    }
    else proc->thread_list = new;

    proc->thread_count++;

    return new;
}

void process_delete_thread(process_t* proc, thread_t* thread) {
    if(thread->state == PROCESS_STATE_ACTIVE) return;

    // remove thread from thread list
    if(thread->prev)
        thread->prev->next = thread->next;
    if(thread->next)
        thread->next->prev = thread->prev;

    // move thread list
    if(thread == proc->thread_list)
        proc->thread_list = thread->next;

    // free stack
    if(!is_user_proc(proc))
        kfree((void*)thread->stack_addr);
    else
        heap_free((heap_t*)UHEAP_START, (void*)thread->stack_addr);

    kfree(thread);

    proc->thread_count--;
}
