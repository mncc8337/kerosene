#include "process.h"
#include "filesystem.h"
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
    proc->state = PROCESS_STATE_READY;
    proc->alive_ticks = 0;
    proc->sleep_ticks = 0;
    proc->next = NULL;

    if(!is_user) proc->page_directory = vmmngr_get_kernel_page_directory();
    else {
        // only users need to have a separate page directory
        proc->page_directory = vmmngr_alloc_page_directory();
        if(!proc->page_directory) {
            kfree(proc);
            return NULL;
        }

    }

    if(!is_user) {
        proc->file_descriptor_table = vfs_get_kernel_file_descriptor_table();
        proc->file_count = vfs_get_kernel_file_count();
        proc->cwd = &vfs_getfs(RAMFS_DISK)->root_node;
    }
    else {
        // create a file descriptor table
        void* fdt = kmalloc(sizeof(file_descriptor_entry_t) * MAX_FILE);
        if(!fdt) {
            vmmngr_free_page_directory(proc->page_directory);
            kfree(proc);
            return NULL;
        }
        proc->file_descriptor_table = fdt;
        proc->file_count = 0;
        // TODO: set cwd
        proc->cwd = NULL;
    }

    regs_t* regs = &proc->regs;
    regs->eip = eip;
    regs->eflags = DEFAULT_EFLAGS;
    regs->ebp = 0;
    if(is_user) {
        // switch page directory to create user heap
        vmmngr_switch_page_directory(proc->page_directory);

        heap_t* heap = heap_new(UHEAP_START, UHEAP_INITIAL_SIZE, UHEAP_MAX_SIZE, 0b00);
        if(!heap) {
            vmmngr_switch_page_directory(vmmngr_get_kernel_page_directory());
            vmmngr_free_page_directory(proc->page_directory);
            kfree(proc->file_descriptor_table);
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
        proc->stack_addr = (uint32_t)heap_alloc(heap, DEFAULT_STACK_SIZE, false);

        vmmngr_switch_page_directory(vmmngr_get_kernel_page_directory());
    }
    else {
        regs->cs = 0x08; // kernel code selector
        regs->ds = 0x10; // kernel data selector
        regs->es = regs->ds;
        regs->fs = regs->ds;
        regs->gs = regs->ds;
        regs->ss = regs->ds;
        proc->stack_addr = (uint32_t)kmalloc(DEFAULT_STACK_SIZE);

        if(!proc->stack_addr) {
            kfree(proc);
            return NULL;
        }
    }
    regs->useresp = proc->stack_addr + DEFAULT_STACK_SIZE;

    process_count++;

    return proc;
}

void process_delete(process_t* proc) {
    // kernel resources are shared so dont touch them
    if(is_user_proc(proc)) {
        // TODO: close all opended files
        vmmngr_free_page_directory(proc->page_directory);
    }

    kfree(proc);
}
