#include <process.h>
#include <filesystem.h>
#include <system.h>
#include <mem.h>
#include <sys/files.h>

#include <stdlib.h>

static unsigned process_count = 0;

// from kernel.c
extern process_t* kernel_process;

process_t* process_new(uint32_t eip, bool is_user, fs_node_t* cwd) {
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    if(!proc) return NULL;

    // save active pd for reverting
    page_directory_t* active_pd = vmmngr_get_page_directory();

    proc->state = PROCESS_STATE_READY;
    proc->alive_ticks = 0;
    proc->sleep_ticks = 0;
    proc->next = NULL;
    proc->is_user = is_user;
    proc->cwd = cwd ? cwd : &vfs_getfs(RAMFS_DISK)->root_node;

    if(!is_user) {
        proc->page_directory = (page_directory_t*)KERNEL_PAGE_DIRECTORY;
        proc->file_descriptor_table = vfs_get_kernel_file_descriptor_table();
        proc->file_count = vfs_get_kernel_file_count();
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
    }

    // allocate the new stack
    size_t stack_size;
    
    // disable interrupts before switching page directory
    // so the scheduler won't switch task and clobber our CR3
    uint32_t eflags_cr3;
    asm volatile("pushf; pop %0; cli" : "=r"(eflags_cr3));

    if(is_user) {
        // switch page directory to create user heap
        vmmngr_switch_page_directory(proc->page_directory);
        bool clean_up = false;

        heap_t* heap = heap_new(UHEAP_START, UHEAP_INITIAL_SIZE, UHEAP_MAX_SIZE, 0b00);
        if(!heap) {
            clean_up = true;
            goto clean_up;
        }

        proc->stack_addr = (uint32_t)heap_alloc(heap, USER_STACK_SIZE, false);
        if(!proc->stack_addr) {
            clean_up = true;
            goto clean_up;
        }
        stack_size = USER_STACK_SIZE;

        proc->tss_esp0 = (uint32_t)kmalloc(KERNEL_STACK_SIZE);
        if(!proc->tss_esp0) {
            clean_up = true;
            goto clean_up;
        }
        proc->tss_esp0 += KERNEL_STACK_SIZE;

        clean_up:
        if(clean_up) {
            vmmngr_switch_page_directory(active_pd);
            vmmngr_free_page_directory(proc->page_directory);
            kfree(proc->file_descriptor_table);
            kfree(proc);
            asm volatile("push %0; popf" : : "r"(eflags_cr3));
            return NULL;
        }
    } else {
        proc->stack_addr = (uint32_t)kmalloc(KERNEL_STACK_SIZE);
        if(!proc->stack_addr) {
            kfree(proc);
            asm volatile("push %0; popf" : : "r"(eflags_cr3));
            return NULL;
        }
        stack_size = KERNEL_STACK_SIZE;

        // unused for kernel process
        proc->tss_esp0 = 0;
    }

    // shift the stack top down by sizeof(virtual_addr_t) to reserve a slot for
    // the root funcion dummy return address
    // why? when the root function execute its standard prolouge (push ebp; move ebp, esp)
    // its stack frame expects a return address at ebp + sizeof(virtual_address_t)
    // which originally aligned with stack_top
    // without the shift, reading ebp + sizeof(virtual_addr_t) would access the adj memory
    // belong to the next heap block, which is the magic number field as the return address
    // (or worse, an unmapped memory region)
    // so we shift it by sizeof(virtual_addr_t) to safely contain
    // the stack within the allocated block
    // and set stack_top to 0 for the backtracer to return gracefully
    uint32_t stack_top = proc->stack_addr + stack_size - sizeof(virtual_addr_t);
    *(uint32_t*)stack_top = 0;

    // `push` default register states
    regs_t* regs;
    if(is_user) {
        regs = (regs_t*)(proc->tss_esp0 - sizeof(regs_t));
        regs->cs = 0x1b; // user code selector
        regs->ds = 0x23; // user data selector
        regs->es = regs->ds;
        regs->fs = regs->ds;
        regs->gs = regs->ds;
        regs->ss = regs->ds;
        regs->useresp = stack_top;
    } else {
        // upon `iret` from ring 0 (kernel)
        // it only pops eip, cs and eflags
        // while in ring 3 (user)
        // it pops eip, cs, eflags, useresp and ss
        // so the regs pointer is different
        // and we dont set useresp and ss here
        regs = (regs_t*)(stack_top - sizeof(regs_t) + 8);
        regs->cs = 0x08; // kernel code selector
        regs->ds = 0x10; // kernel data selector
        regs->es = regs->ds;
        regs->fs = regs->ds;
        regs->gs = regs->ds;
    }
    regs->eax = 0;
    regs->ebx = 0;
    regs->ecx = 0;
    regs->edx = 0;
    regs->esi = 0;
    regs->edi = 0;
    regs->int_no = 0;
    regs->err_code = 0;
    regs->eip = eip;
    regs->eflags = DEFAULT_EFLAGS;
    regs->ebp = 0;

    proc->saved_esp = (uint32_t)regs;

    // restore to active pd
    vmmngr_switch_page_directory(active_pd);

    // safely increment proc count while interrupts are still disabled
    proc->id = ++process_count;

    // re-enable interrupts
    asm volatile("push %0; popf" : : "r"(eflags_cr3));

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

        // should always success
        file_open(fdt + SYSFILE_FD_STDIN,  vfs_get_stdin(),  "r");
        file_open(fdt + SYSFILE_FD_STDOUT, vfs_get_stdout(), "a");

        fdt[SYSFILE_FD_STDIN].node->refcount++;
        fdt[SYSFILE_FD_STDOUT].node->refcount++;

        proc->file_count = 2;
    }

    return proc;
}

process_t* process_make_idle() {
    // make the process that called this an idle process
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    if(!proc) return NULL;

    proc->state = PROCESS_STATE_READY;
    proc->alive_ticks = 0;
    proc->sleep_ticks = 0;
    proc->next = NULL;

    proc->page_directory = (page_directory_t*)KERNEL_PAGE_DIRECTORY;
    proc->file_descriptor_table = vfs_get_kernel_file_descriptor_table();
    proc->file_count = vfs_get_kernel_file_count();
    proc->cwd = &vfs_getfs(RAMFS_DISK)->root_node;

    // no need to allocate a new stack
    // or set up registers

    // prevent interrupts to safely increment proc count
    uint32_t eflags;
    asm volatile("pushf; pop %0; cli" : "=r"(eflags));
    proc->id = 1;
    process_count = 1;
    asm volatile("push %0; popf" : : "r"(eflags));

    return proc;
}

void process_delete(process_t* proc) {
    if(proc != kernel_process) {
        vmmngr_free_page_directory(proc->page_directory);

        kfree((void*)proc->tss_esp0 - KERNEL_STACK_SIZE);

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
            file_sync(fde);

            fde->node->refcount--;
            vfs_cleanup_node_tree(fde->node);

            fde->node = NULL;
            proc->file_count--;

            i++;
        }
        kfree(proc->file_descriptor_table);
    } else {
        // kernel resources are shared so dont touch them
        // only delete the stack
        kfree((void*)proc->stack_addr);
    }

    kfree(proc);
}
