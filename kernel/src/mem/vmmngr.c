#include "mem.h"

#define PAGE_FRAME_BITS 0x7ffff000

// our page table VIRTUAL address can be get by adding 0xffc00000 (4*1024*1024*1023)
// with page directory index multiply by page size
// we can do this because we have recursive paging (see kernel_entry.asm)
// we need to use virtual address because accessing a physical address will result in a page fault
#define PAGE_TABLE_ADDR(addr) (page_table_t*)(0xffc00000 + PAGE_DIRECTORY_INDEX(addr) * MMNGR_PAGE_SIZE)

static page_directory_t* current_page_directory = 0;

static void page_entry_set_frame(uint32_t* pe, physical_addr_t addr) {
    *pe = (*pe & ~PAGE_FRAME_BITS) | addr;
}
static void page_entry_add_attrib(uint32_t* pe, uint16_t attrib) {
    *pe |= (attrib & 0xfff);
}
static void page_entry_del_attrib(uint32_t* pe, uint16_t attrib) {
    *pe &= ~(attrib & 0xfff);
}

page_directory_t* vmmngr_get_directory() {
    return current_page_directory;
}

physical_addr_t vmmngr_to_physical_addr(page_directory_t* pd, virtual_addr_t virt) {
    pde_t* pde = PAGE_DIRECTORY_LOOKUP(pd, virt);
    if(!(*pde & PDE_PRESENT)) return 0;

    page_table_t* table = PAGE_TABLE_ADDR(PAGE_DIRECTORY_INDEX((uint32_t)virt));
    pte_t* pte = PAGE_TABLE_LOOKUP(table, virt);
    if(!(*pte & PTE_PRESENT)) return 0;

    return (physical_addr_t)(*pte & PAGE_FRAME_BITS);
}

MEM_ERR vmmngr_map(page_directory_t* page_directory, physical_addr_t phys, virtual_addr_t virt, unsigned flags) {
    if(page_directory == NULL) page_directory = current_page_directory;
    pde_t* pde = PAGE_DIRECTORY_LOOKUP(page_directory, virt);

    bool new_table = false;
    // if the page table is not present then allocate it
    if(!(*pde & PDE_PRESENT)) {
        page_table_t* table = (page_table_t*)pmmngr_alloc_block();
        if(!table) return ERR_MEM_OOM;
        new_table = true;

        // create a new entry
        page_entry_add_attrib(pde, PDE_PRESENT);
        page_entry_set_frame(pde, (physical_addr_t)table);
    }

    page_table_t* table = PAGE_TABLE_ADDR(virt);

    // clear the table if we have just created it
    if(new_table)
        for(unsigned i = 0; i < 1024; i++) table->entry[i] = 0x0;

    pte_t* pte = PAGE_TABLE_LOOKUP(table, virt);
    page_entry_add_attrib(pte, PTE_PRESENT | flags);
    page_entry_set_frame(pte, phys);

    vmmngr_flush_tlb_entry(virt);

    return ERR_MEM_SUCCESS;
}

MEM_ERR vmmngr_unmap(page_directory_t* page_directory, virtual_addr_t virt) {
    if(page_directory == NULL) page_directory = current_page_directory;
    pde_t* pde = PAGE_DIRECTORY_LOOKUP(current_page_directory, virt);
    if(!(*pde & PDE_PRESENT)) return ERR_MEM_UNMAPPED;

    page_table_t* table = PAGE_TABLE_ADDR(PAGE_DIRECTORY_INDEX((uint32_t)virt));
    pte_t* pte = PAGE_TABLE_LOOKUP(table, virt);
    if(!(*pte & PTE_PRESENT)) return ERR_MEM_UNMAPPED;

    physical_addr_t phys = (physical_addr_t)(*pte & PAGE_FRAME_BITS);
    pmmngr_free_block((void*)phys);
    // clear all attrib
    *pte = 0x0;

    vmmngr_flush_tlb_entry(virt);

    return ERR_MEM_SUCCESS;
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

page_directory_t* vmmngr_alloc_page_directory() {
    page_directory_t* pd = (page_directory_t*)pmmngr_alloc_block();
    if(pd == NULL) return NULL;

    vmmngr_map(NULL, (physical_addr_t)pd, VMMNGR_RESERVED, PTE_PRESENT | PTE_WRITABLE);
    page_directory_t* pd_virt = (page_directory_t*)VMMNGR_RESERVED;

    // TODO: copy the kernel page directory instead of clearing
    for(unsigned i = 0; i < 1024; i++) pd_virt->entry[i] = 0;

    // set final entry to itself for recursive paging
    pde_t* pde = &pd_virt->entry[1023];
    page_entry_add_attrib(pde, PDE_PRESENT | PDE_WRITABLE);
    page_entry_set_frame(pde, (physical_addr_t)pd);

    return pd;
}

MEM_ERR vmmngr_switch_page_directory(page_directory_t* dir, physical_addr_t pdbr) {
    if(!dir) return ERR_MEM_INVALID_DIR;

    current_page_directory = dir;
    asm volatile("mov %0, %%cr3" : : "r" (pdbr));
    return ERR_MEM_SUCCESS;
}

void vmmngr_flush_tlb_entry(virtual_addr_t addr) {
	asm volatile("invlpg (%0)" : : "b" ((void*)addr) : "memory");
}

extern void* kernel_page_directory;
void vmmngr_init() {
    // reuse the page directory in kernel_entry.asm
    current_page_directory = (page_directory_t*)(&kernel_page_directory);
}
