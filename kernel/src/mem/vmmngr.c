#include "mem.h"

extern void load_page_directory(physical_addr_t);

static pdir* current_directory = 0;
static physical_addr_t phys_pdir;

void* _get_physaddr(void *virtualaddr) {
    virtual_addr_t pdindex = (virtual_addr_t)virtualaddr >> 22;
    virtual_addr_t ptindex = (virtual_addr_t)virtualaddr >> 12 & 0x03FF;

    virtual_addr_t *pd = (virtual_addr_t*)0xFFFFF000;
    // Here you need to check whether the PD entry is present.

    virtual_addr_t *pt = ((virtual_addr_t*)0xFFC00000) + (0x400 * pdindex);
    // Here you need to check whether the PT entry is present.

    return (void *)((pt[ptindex] & ~0xFFF) + ((virtual_addr_t)virtualaddr & 0xFFF));
}

pte_t* vmmngr_ptable_lookup_entry(ptable* p, virtual_addr_t addr) {
    return &(p->entry[PAGE_TABLE_INDEX(addr)]);
}

pde_t* vmmngr_pdirectory_lookup_entry(pdir* p, virtual_addr_t addr) {
    return &(p->entry[PAGE_DIRECTORY_INDEX(addr)]);
}

MEM_ERR vmmngr_switch_pdirectory(pdir* dir) {
    if(!dir) return ERR_MEM_INVALID_DIR;

    current_directory = dir;
    load_page_directory((physical_addr_t)phys_pdir);
    return ERR_MEM_SUCCESS;
}

pdir* vmmngr_get_directory() {
    return current_directory;
}

void vmmngr_flush_tlb_entry(void* addr) {
	asm volatile("invlpg (%0)" : : "b" (addr) : "memory");
}

MEM_ERR vmmngr_alloc_page(pte_t* e) {
    void* p = pmmngr_alloc_block();
    if(!p) return ERR_MEM_OOM;

    pte_set_frame(e, (physical_addr_t)p);
    pte_add_attrib(e, PTE_PRESENT);

    return ERR_MEM_SUCCESS;
}

void vmmngr_free_page(pte_t* e) {
    void* p = (void*)pte_pfn(*e);
    if(p) pmmngr_free_block(p);

    pte_del_attrib(e, PTE_PRESENT);
}

MEM_ERR vmmngr_map_page(physical_addr_t phys, virtual_addr_t virt) {
    pdir* pdirectory = vmmngr_get_directory();

    int pd_index = PAGE_DIRECTORY_INDEX((uint32_t)virt);
    pde_t* pde = &pdirectory->entry[pd_index];

    bool new_table = false;
    if((*pde & PTE_PRESENT) != PTE_PRESENT) {
        // if page table not present then allocate it
        ptable* table = (ptable*)pmmngr_alloc_block();
        if(!table) return ERR_MEM_OOM;
        new_table = true;

        // create a new entry
        pde_add_attrib(pde, PDE_PRESENT);
        pde_add_attrib(pde, PDE_WRITABLE);
        pde_set_frame(pde, (physical_addr_t)table);

   }

    // unsigned table_phys = PAGE_GET_PHYSICAL_ADDRESS(pde);

    // our page table VIRTUAL address can be get by adding 0xffc00000 (4*1024*1024*1023)
    // with page directory index multiply by page size
    // we can do this because we have recursive paging (see kernel_entry.asm)
    // we need to use virtual address because accessing physical address will result in a page fault
    ptable* table = (ptable*)(0xffc00000 + pd_index * MMNGR_PAGE_SIZE);

    // clear the table if we have just created it
    if(new_table) {
        for(unsigned i = 0; i < 1024; i++)
            table->entry[i] = 0x0;
    }

    pte_t* pte = &table->entry[PAGE_TABLE_INDEX((uint32_t)virt)];
    pte_add_attrib(pte, PTE_PRESENT);
    pte_add_attrib(pte, PTE_WRITABLE);
    pte_set_frame(pte, phys);

    vmmngr_flush_tlb_entry((void*)virt);

    return ERR_MEM_SUCCESS;
}

extern void* page_directory;
extern uint32_t kernel_size;
MEM_ERR vmmngr_init() {
    // reuse the page_directory in kernel_entry
    // note that the page_directory is allocated in the kernel
    // and the kernel is mapped to virtual memory
    // so we can access it in any time
    current_directory = (pdir*)(&page_directory);
    phys_pdir = (unsigned)&page_directory - VMBASE_KERNEL;

    return ERR_MEM_SUCCESS;
}
