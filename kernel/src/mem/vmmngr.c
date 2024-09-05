#include "mem.h"

#define PAGE_FRAME_BITS 0x7ffff000

static page_directory_t* current_page_directory = 0;
static physical_addr_t pdbr;

static void page_entry_set_frame(uint32_t* pe, physical_addr_t addr) {
    *pe = (*pe & ~PAGE_FRAME_BITS) | addr;
}
static void page_entry_add_attrib(uint32_t* pe, uint16_t attrib) {
    *pe |= (attrib & 0xfff);
}
static void page_entry_del_attrib(uint32_t* pe, uint16_t attrib) {
    *pe &= ~(attrib & 0xfff);
}

static page_table_t* get_page_table(unsigned pd_index) {
    // our page table VIRTUAL address can be get by adding 0xffc00000 (4*1024*1024*1023)
    // with page directory index multiply by page size
    // we can do this because we have recursive paging (see kernel_entry.asm)
    // we need to use virtual address because accessing physical address will result in a page fault
    return (page_table_t*)(0xffc00000 + pd_index * MMNGR_PAGE_SIZE);
}

pte_t* vmmngr_ptable_lookup_entry(page_table_t* p, virtual_addr_t addr) {
    return &(p->entry[PAGE_TABLE_INDEX(addr)]);
}

pde_t* vmmngr_pdirectory_lookup_entry(page_directory_t* p, virtual_addr_t addr) {
    return &(p->entry[PAGE_DIRECTORY_INDEX(addr)]);
}

page_directory_t* vmmngr_get_directory() {
    return current_page_directory;
}

MEM_ERR vmmngr_alloc_page(pte_t* pte) {
    void* p = pmmngr_alloc_block();
    if(!p) return ERR_MEM_OOM;

    page_entry_set_frame(pte, (physical_addr_t)p);
    page_entry_add_attrib(pte, PTE_PRESENT);

    return ERR_MEM_SUCCESS;
}

void vmmngr_free_page(pte_t* pte) {
    void* p = (void*)(*pte & PAGE_FRAME_BITS);
    if(p) pmmngr_free_block(p);

    page_entry_del_attrib(pte, PTE_PRESENT);
}

MEM_ERR vmmngr_map_page(physical_addr_t phys, virtual_addr_t virt, unsigned flags) {
    int pd_index = PAGE_DIRECTORY_INDEX((uint32_t)virt);
    pde_t* pde = &current_page_directory->entry[pd_index];

    bool new_table = false;
    // if page table not present then allocate it
    if(!(*pde & PTE_PRESENT)) {
        page_table_t* table = (page_table_t*)pmmngr_alloc_block();
        if(!table) return ERR_MEM_OOM;
        new_table = true;

        // create a new entry
        page_entry_add_attrib(pde, PDE_PRESENT);
        page_entry_set_frame(pde, (physical_addr_t)table);
   }

    page_table_t* table = get_page_table(pd_index);

    // clear the table if we have just created it
    if(new_table) {
        for(unsigned i = 0; i < 1024; i++)
            table->entry[i] = 0x0;
    }

    pte_t* pte = &table->entry[PAGE_TABLE_INDEX((uint32_t)virt)];
    page_entry_add_attrib(pte, PTE_PRESENT | flags);
    page_entry_set_frame(pte, phys);

    vmmngr_flush_tlb_entry(virt);

    return ERR_MEM_SUCCESS;
}

physical_addr_t vmmngr_to_physical_addr(virtual_addr_t virt) {
    int pd_index = PAGE_DIRECTORY_INDEX((uint32_t)virt);
    pde_t* pde = &current_page_directory->entry[pd_index];

    if((*pde & PTE_PRESENT) != PTE_PRESENT) return 0;

    page_table_t* table = get_page_table(pd_index);
    pte_t* pte = vmmngr_ptable_lookup_entry(table, virt);

    if(!(*pte & PTE_PRESENT)) return 0;
    return (physical_addr_t)(*pte & PAGE_FRAME_BITS);
}

MEM_ERR vmmngr_switch_pdirectory(page_directory_t* dir) {
    if(!dir) return ERR_MEM_INVALID_DIR;

    current_page_directory = dir;
    vmmngr_load_page_directory((physical_addr_t)pdbr);
    return ERR_MEM_SUCCESS;
}

void vmmngr_load_page_directory(physical_addr_t addr) {
    asm volatile("mov %0, %%cr3" : : "r" (addr));
}

void vmmngr_flush_tlb_entry(virtual_addr_t addr) {
	asm volatile("invlpg (%0)" : : "b" ((void*)addr) : "memory");
}

extern void* page_directory;
MEM_ERR vmmngr_init() {
    // reuse the page_directory in kernel_entry
    // note that the page_directory is allocated in the kernel
    // and the kernel is mapped to virtual memory
    // so we can access it in any time
    current_page_directory = (page_directory_t*)(&page_directory);
    pdbr = (unsigned)&page_directory - VMBASE_KERNEL;

    return ERR_MEM_SUCCESS;
}
