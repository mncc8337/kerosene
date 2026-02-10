#include <process.h>
#include <filesystem.h>
#include <stdlib.h>
#include <system.h>
#include <mem.h>

#define DEFAULT_EFLAGS 0x202
#define DEFAULT_STACK_SIZE 16 * 1024

static unsigned process_count = 0;

// from kernel.c
extern process_t* kernel_process;

process_t* process_new(uint32_t eip, int priority, bool is_user) {
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    if(!proc) return NULL;

    process_t* parent_proc = scheduler_get_current_process();

    // user can only create user proc
    if(parent_proc != kernel_process && !is_user)
        return NULL;

    proc->priority = priority;
    proc->state = PROCESS_STATE_READY;
    proc->alive_ticks = 0;
    proc->sleep_ticks = 0;
    proc->next = NULL;

    if(!is_user) {
        proc->page_directory = vmmngr_get_kernel_page_directory();
        proc->file_descriptor_table = vfs_get_kernel_file_descriptor_table();
        proc->file_count = vfs_get_kernel_file_count();
        proc->cwd = &vfs_getfs(RAMFS_DISK)->root_node;
    } else {
        // create a new page directory
        proc->page_directory = vmmngr_alloc_page_directory();
        if(!proc->page_directory) {
            kfree(proc);
            return NULL;
        }

        // create a file descriptor table
        void* fdt = kmalloc(sizeof(file_description_t) * MAX_FILE);
        if(!fdt) {
            vmmngr_free_page_directory(proc->page_directory);
            kfree(proc);
            return NULL;
        }
        proc->file_descriptor_table = fdt;
        proc->file_count = 0;

        proc->cwd = parent_proc->cwd;
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
        proc->stack_addr = (uint32_t)heap_alloc(heap, DEFAULT_STACK_SIZE, false);
        vmmngr_switch_page_directory(vmmngr_get_kernel_page_directory());

        if(!proc->stack_addr) {
            vmmngr_free_page_directory(proc->page_directory);
            kfree(proc->file_descriptor_table);
            kfree(proc);
            return NULL;
        }

    } else {
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

    // prevent interrupts to safely increment proc count
    unsigned long eflags;
    asm volatile("pushf; pop %0" : "=r"(eflags));
    asm volatile("cli");
    proc->id = ++process_count;
    asm volatile("push %0; popf" : : "r"(eflags));

    // create standard files
    if(is_user) {
        // fs_t* ramfs = vfs_getfs(RAMFS_DISK);

        fs_node_t* sysproc_dir = vfs_get_proc_dir();

        char buff[10];
        itoa(proc->id, buff, 10);


        fs_node_t* proc_dir;
        if(vfs_find_and_create_node(buff, sysproc_dir, &proc_dir, true, false)) {
            process_delete(proc);
            return NULL;
        }

        file_description_t* fdt = proc->file_descriptor_table;
        file_description_t* stdout = fdt + 0;
        file_description_t* stdin = fdt + 1;

        // should always success
        file_open(stdout, vfs_get_glbout(), "a");
        file_open(stdin, vfs_get_glbin(), "a+");

        fdt[0].node->refcount++;
        fdt[1].node->refcount++;

        proc->file_count = 2;
    }

    return proc;
}

void process_delete(process_t* proc) {
    // kernel resources are shared so dont touch them
    if(proc != kernel_process) {
        vmmngr_free_page_directory(proc->page_directory);

        // close opened file
        unsigned i = 0;
        while(proc->file_count && i < MAX_FILE) {
            file_description_t* fde = proc->file_descriptor_table + i;
            if(!(fde->node)) {
                i++;
                continue;
            }

            // TODO:
            // wait for IO op to be done
            // before closing
            file_close(fde);

            fde->node->refcount--;
            vfs_cleanup_node_tree(fde->node);

            fde->node = NULL;
            proc->file_count--;

            i++;
        }
        kfree(proc->file_descriptor_table);
    }

    kfree(proc);
}
