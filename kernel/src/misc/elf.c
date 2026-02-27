#include <misc/elf.h>

#include <mem.h>
#include <filesystem.h>

#include <string.h>

static int errcode;

elf_section_header_t* elf_get_shtab(elf_header_t* eh) {
    return (elf_section_header_t*)((void*)eh + eh->sh_offset);
}

elf_program_header_t* elf_get_phtab(elf_header_t* eh) {
    return (elf_program_header_t*)((void*)eh + eh->ph_offset);
}

char* elf_get_shstrtab(elf_header_t* eh) {
    elf_section_header_t* shstrtab_h = elf_get_shtab(eh) + eh->shstrndx;
    return (char*)((void*)eh + shstrtab_h->offset);
}

ELF_ERR elf_validate(elf_header_t* elf_header) {
    if(elf_header->magic != ELF_MAGIC)
        return ERR_ELF_NOT_ELF;
    if(elf_header->bits != ELF_32_BITS)
        return ERR_ELF_NOT_32_BITS;
    if(elf_header->endianess != ELF_LITTLE_ENDIAN)
        return ERR_ELF_NOT_LITTLE_ENDIAN;
    if(elf_header->instruction_set != ELF_INSSET_X86)
        return ERR_ELF_NOT_x86;

    return ERR_ELF_SUCCESS;
}

int elf_get_err() {
    return errcode;
}

ELF_ERR elf_load(fs_node_t* node, void* addr, page_directory_t* pd, uint32_t* entry) {
    file_description_t f;
    errcode = file_open(&f, node, "r");
    if(errcode)
        return ERR_ELF_FILE_ERROR;

    size_t read_size;
    errcode = file_read(&f, addr, node->size, &read_size);
    file_sync(&f);
    if(errcode != ERR_FS_SUCCESS && errcode != ERR_FS_EOF)
        return ERR_ELF_FILE_ERROR;

    elf_header_t* elf_header = (elf_header_t*)addr;
    int elf_err = elf_validate(elf_header);
    if(elf_err) return elf_err;

    elf_program_header_t* phtab = elf_get_phtab(elf_header);

    // find first and last loadable segment
    size_t minaddr = 0xffffffff;
    size_t maxaddr = 0x0;
    size_t last_segment_size = 0;
    for(unsigned i = 0; i < elf_header->ph_entry_count; i++) {
        elf_program_header_t* ph = &phtab[i];
        if(ph->type != ELF_PT_LOAD) continue;

        if(ph->vaddr <= minaddr) {
            minaddr = ph->vaddr;
        }
        if(ph->vaddr >= maxaddr) {
            maxaddr = ph->vaddr;
            last_segment_size = ph->mem_segment_size;
        }
    }

    // page align minaddr
    minaddr -= minaddr % MMNGR_PAGE_SIZE;

    size_t prog_size = maxaddr + last_segment_size - minaddr;
    size_t required_pages = prog_size / MMNGR_PAGE_SIZE;
    required_pages += (prog_size % MMNGR_PAGE_SIZE) > 0;

    unsigned flags = PTE_WRITABLE;
    if(pd != vmmngr_get_kernel_page_directory())
        flags |= PTE_USER;

    // TODO: check and apply flags for each section
    physical_addr_t phys = (physical_addr_t)pmmngr_alloc_multi_block(required_pages);
    if(!phys) return ERR_ELF_OOM;
    for(unsigned i = 0; i < required_pages; i++) {
        MEM_ERR mem_err = vmmngr_map(pd, phys + i * MMNGR_PAGE_SIZE, minaddr + i * MMNGR_PAGE_SIZE, flags);
        if(mem_err) return ERR_ELF_OOM; // the only error returned by vmmngr_map() is OOM
    }

    // now all the vaddrs in PHs are usable
    // switch to the modified pd to set stuff

    vmmngr_switch_page_directory(pd);

    for(unsigned i = 0; i < elf_header->ph_entry_count; i++) {
        elf_program_header_t* ph = &phtab[i];
        if(ph->type != ELF_PT_LOAD) continue;

        memcpy((void*)ph->vaddr, (void*)addr + ph->offset, ph->file_segment_size);

        // set the remain bytes to zero
        if(ph->mem_segment_size > ph->file_segment_size)
            memset((void*)ph->vaddr + ph->file_segment_size, 0, ph->mem_segment_size - ph->file_segment_size);
    }

    vmmngr_switch_page_directory(vmmngr_get_kernel_page_directory());

    *entry = elf_header->program_entry;
    return ERR_ELF_SUCCESS;
}

ELF_ERR elf_load_to_proc(char* path, process_t* proc) {
    fs_t* fs = vfs_getfs(0);
    if(!fs) {
        errcode = ERR_FS_FAILED;
        return ERR_ELF_FILE_ERROR;
    }

    fs_node_t elf;
    errcode = fs_find(&fs->root_node, path, &elf);
    if(errcode)
        return ERR_ELF_FILE_ERROR;

    void* addr = kmalloc(elf.size);
    if(!addr)
        return ERR_ELF_OOM;

    uint32_t entry;
    errcode = elf_load(&elf, addr, proc->page_directory, &entry);
    kfree(addr);
    if(errcode)
        return errcode;

    // printf("jumping to entry (0x%x)\n", entry);
    // int (*prog)(void) = (void*)entry;
    // int exit_code = prog();
    // printf("program exited with code %d\n", exit_code);
    
    vmmngr_switch_page_directory(proc->page_directory);
    regs_t* regs = (regs_t*)proc->saved_esp;
    regs->eip = entry;
    vmmngr_switch_page_directory(vmmngr_get_kernel_page_directory());

    return ERR_ELF_SUCCESS;
}
