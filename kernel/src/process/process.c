#include <process.h>
#include <filesystem.h>
#include <stdint.h>
#include <system.h>
#include <mem.h>

#include <stdlib.h>
#include <string.h>
#include <sys/files.h>

static unsigned process_count = 0;

// in process_new(),
// args may allocated in lower half of the current process's page dir, which is not available
// when switched to the new process (when creating new process)
// so we need to copy it into the kernel memory heap, which is shared
// the same reason for envs
static char args_buffer[ARGS_MAX_LEN];
static char envs_buffer[ENVS_MAX_LEN];

// parse null-separated, double null-terminated strings into string map
// like argc and envp
static void parse_null_separated_strings(
    uint32_t* useresp,
    const unsigned max_len,
    const char* strings,
    unsigned* count,
    char*** ptr_map
) {
    *count = 0;
    *ptr_map = NULL;

    if(!strings || strings[0] == '\0') return;

    // find the total byte length of all strings combined
    unsigned strings_len = 0;
    unsigned string_count = 0;
    while(strings_len < max_len) {
        // go to the next string
        while(strings_len < max_len && strings[strings_len] != '\0')
            strings_len++;

        if(strings_len < max_len - 1) {
            strings_len++; // skip '\0'
            string_count++;

            // double null-termination
            if(strings[strings_len] == '\0') {
                strings_len++;
                break;
            }
        } else break;
    }
    if(string_count == 0) return;

    // allocate space on stack for the strings and copy them here
    *useresp -= strings_len;
    char* trunc_strings = (char*)(*useresp);
    memcpy(trunc_strings, strings, strings_len);

    // allocate space on stack for the map
    *useresp -= sizeof(char*) * (string_count + 1);
    char** string_map = (char**)(*useresp);

    // fill the map
    char* current_str = trunc_strings;
    for(unsigned i = 0; i < string_count; i++) {
        string_map[i] = current_str;
        
        // go to the next string
        while(*current_str != '\0') current_str++;
        current_str++;
    }
    string_map[string_count] = NULL;

    *count = string_count;
    *ptr_map = string_map;
}


static void copy_double_null(char* dest, const char* src, size_t max_len) {
    if(!src) return;
    for(size_t i = 0; i < max_len; i++) {
        dest[i] = src[i];
        if(i > 0 && src[i] == '\0' && src[i-1] == '\0') break;
    }
}

process_t* process_new(
    uint32_t eip,
    bool is_user,
    page_directory_t* pagedir,
    fs_node_t* cwd,
    char* args,
    char* envs
) {
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    if(!proc) return NULL;

    if(is_user) {
        // copy args and envs to the shared memory
        if(args) copy_double_null(args_buffer, args, ARGS_MAX_LEN);
        if(envs) copy_double_null(envs_buffer, envs, ENVS_MAX_LEN);
    }

    // save active pd for reverting
    page_directory_t* active_pd = vmmngr_get_page_directory();

    proc->state = PROCESS_STATE_READY;
    proc->alive_ticks = 0;
    proc->sleep_ticks = 0;
    proc->is_user = is_user;
    proc->cwd = cwd ? cwd : &vfs_get_ramfs()->root_node;

    proc->attached_from = NULL;

    if(!is_user) {
        proc->page_directory = (page_directory_t*)KERNEL_PAGE_DIRECTORY;
    } else {
        proc->page_directory = pagedir;
    }

    // create a file descriptor table
    void* fdt = kmalloc(sizeof(file_description_t) * MAX_FILE);
    if(!fdt) {
        kfree(proc);
        return NULL;
    }
    proc->file_descriptor_table = fdt;
    proc->file_count = 0;

    // allocate the new stack
    size_t stack_size;
    
    // disable interrupts before switching page directory
    // so the scheduler won't switch task and clobber our CR3
    uint32_t eflags_cr3;
    asm volatile("pushf; pop %0; cli" : "=r"(eflags_cr3));

    if(is_user) {
        // switch page directory to set up the stack
        vmmngr_switch_page_directory(proc->page_directory);
        bool clean_up = false;

        proc->stack_addr = (uint32_t)USER_STACK_TOP - USER_STACK_SIZE;
        if(!proc->stack_addr) {
            clean_up = true;
            goto clean_up;
        }
        stack_size = USER_STACK_SIZE;

        // alloc/map the stack
        size_t physical_blocks = USER_STACK_SIZE / MMNGR_PAGE_SIZE;
        physical_addr_t phys = pmmngr_alloc_multi_block(physical_blocks);
        if(!phys) {
            clean_up = true;
            goto clean_up;
        }
        for(unsigned i = 0; i < USER_STACK_SIZE; i += MMNGR_PAGE_SIZE)
            vmmngr_map(
                NULL,
                phys + i,
                USER_STACK_TOP - USER_STACK_SIZE + i,
                PTE_PRESENT | PTE_USER | PTE_WRITABLE
            );

        // NOTE:
        // the phys allocated above is meant to be freed by calling vmmngr_free_page_directory()
        // each page directory is created solely for one user process
        // so this func will not try to free them to avoid double freeing
        // and relies on the caller to do that instead

        proc->tss_esp0 = (uint32_t)kmalloc(KERNEL_STACK_SIZE);
        if(!proc->tss_esp0) {
            clean_up = true;
            goto clean_up;
        }
        proc->tss_esp0 += KERNEL_STACK_SIZE;

        clean_up:
        if(clean_up) {
            vmmngr_switch_page_directory(active_pd);
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

    // fake pushing default register states
    // for the context switching mechanism (see also system/isr.asm isr_common_stub())
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
        // upon `iret` from ring 0 (kernel) it only pops eip, cs and eflags
        // while in ring 3 (user) it also pops useresp and ss
        // we need to shift the regs pointer by 8 bytes to maintain the
        // correct structure
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

    // pass argc, argv, envc and envp to user processes
    if(is_user) {
        unsigned argc;
        unsigned envc;
        char** argv;
        char** envp;
        uint32_t useresp = regs->useresp;
        parse_null_separated_strings(&useresp, ARGS_MAX_LEN, args_buffer, &argc, &argv);
        parse_null_separated_strings(&useresp, ENVS_MAX_LEN, envs_buffer, &envc, &envp);
        regs->eax = argc;
        regs->ebx = (uint32_t)argv;
        regs->ecx = envc;
        regs->edx = (uint32_t)envp;
        regs->useresp = useresp;
    }

    // restore to active pd
    vmmngr_switch_page_directory(active_pd);

    // safely increment proc count while interrupts are still disabled
    proc->id = ++process_count;

    // re-enable interrupts
    asm volatile("push %0; popf" : : "r"(eflags_cr3));

    // create standard files
    if(is_user) {
        fs_node_t* sysproc_dir = vfs_get_proc_dir();

        char buff[10];
        itoa(proc->id, buff, 10);

        fs_node_t* proc_dir;
        if(vfs_find_and_create_node(
            buff,
            sysproc_dir,
            &proc_dir,
            FILE_OPEN_CREATE | FILE_OPEN_EXCLUSIVE,
            FS_NODE_TYPE_DIRECTORY
        )) {
            process_delete(proc);
            return NULL;
        }
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

    proc->page_directory = (page_directory_t*)KERNEL_PAGE_DIRECTORY;

    // idle process does not need to access any files
    // so these can be ignored (until some nasty bugs occurs)
    void* fdt = kmalloc(sizeof(file_description_t) * MAX_FILE);
    proc->file_descriptor_table = fdt;
    proc->file_count = 0;
    proc->cwd = &vfs_get_ramfs()->root_node;

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
    if(proc->is_user) {
        vmmngr_free_page_directory(proc->page_directory);
        kfree((void*)proc->tss_esp0 - KERNEL_STACK_SIZE);
    } else {
        kfree((void*)proc->stack_addr);
    }

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
        node_sync(fde->node);

        fde->node->refcount--;
        vfs_cleanup_node_tree(fde->node);

        fde->node = NULL;
        proc->file_count--;

        i++;
    }
    kfree(proc->file_descriptor_table);

    kfree(proc);
}

void process_set_stdfile(process_t* proc, fs_node_t* stdin, uint32_t stdin_flags, fs_node_t* stdout, uint32_t stdout_flags) {
    file_description_t* fdt = proc->file_descriptor_table;

    // should always success
    file_open(fdt + SYSFILE_FD_STDIN,  stdin,  stdin_flags);
    file_open(fdt + SYSFILE_FD_STDOUT, stdout, stdout_flags);

    fdt[SYSFILE_FD_STDIN].node->refcount++;
    fdt[SYSFILE_FD_STDOUT].node->refcount++;

    proc->file_count = 2;
}
