#include "mem.h"

#include "string.h"

#define PAGE_FRAME_BITS 0x7ffff000

// our page table VIRTUAL address can be get by adding 0xffc00000 (4*1024*1024*1023)
// with page directory index multiply by page size
// we can do this because we have recursive paging (see kernel_entry.asm)
// we need to use virtual address because accessing a physical address will result in a page fault
#define PAGE_TABLE_ADDR(idx) (page_table_t*)(0xffc00000 + idx * MMNGR_PAGE_SIZE)

extern void* kernel_pd_virt;

static page_directory_t* current_page_directory = 0;
static page_directory_t* kernel_page_directory = (page_directory_t*)((unsigned)&kernel_pd_virt - KERNEL_START);

static void page_entry_set_frame(uint32_t* pe, physical_addr_t addr) {
    *pe = (*pe & ~PAGE_FRAME_BITS) | addr;
}
static void page_entry_add_attrib(uint32_t* pe, uint16_t attrib) {
    *pe |= (attrib & 0xfff);
}
static void page_entry_del_attrib(uint32_t* pe, uint16_t attrib) {
    *pe &= ~(attrib & 0xfff);
}

static page_directory_t* mapped_temporal_pd = 0;
static void map_temporary_pd(page_directory_t* pd) {
    if(pd == mapped_temporal_pd) return;
    mapped_temporal_pd = pd;

    vmmngr_map(NULL, (physical_addr_t)pd, VMMNGR_RESERVED, PTE_PRESENT | PTE_WRITABLE);
}

page_directory_t* vmmngr_get_page_directory() {
    return current_page_directory;
}

page_directory_t* vmmngr_get_kernel_page_directory() {
    return kernel_page_directory;
}

physical_addr_t vmmngr_to_physical_addr(page_directory_t* page_directory, virtual_addr_t virt) {
    page_directory_t* virt_pd;
    if(page_directory == NULL) virt_pd = (page_directory_t*)VMMNGR_PD;
    else {
        map_temporary_pd(page_directory);
        virt_pd = (page_directory_t*)VMMNGR_RESERVED;
    }

    pde_t* pde = PAGE_DIRECTORY_LOOKUP(virt_pd, virt);
    if(!(*pde & PDE_PRESENT)) return 0;

    page_table_t* table = PAGE_TABLE_ADDR(PAGE_DIRECTORY_INDEX((uint32_t)virt));
    pte_t* pte = PAGE_TABLE_LOOKUP(table, virt);
    if(!(*pte & PTE_PRESENT)) return 0;

    return (physical_addr_t)(*pte & PAGE_FRAME_BITS);
}

MEM_ERR vmmngr_map(page_directory_t* page_directory, physical_addr_t phys, virtual_addr_t virt, unsigned flags) {
    page_directory_t* virt_pd;
    if(page_directory == NULL) virt_pd = (page_directory_t*)VMMNGR_PD;
    else {
        map_temporary_pd(page_directory);
        virt_pd = (page_directory_t*)VMMNGR_RESERVED;
    }

    pde_t* pde = PAGE_DIRECTORY_LOOKUP(virt_pd, virt);

    bool new_table = false;
    // if the page table is not present then allocate it
    if(!(*pde & PDE_PRESENT)) {
        physical_addr_t new_phys = (physical_addr_t)pmmngr_alloc_block();
        if(!new_phys) return ERR_MEM_OOM;
        new_table = true;

        // create a new entry
        page_entry_add_attrib(pde, flags | PDE_PRESENT); // the first few PDE and PTE flags are the same so we can do this
        page_entry_set_frame(pde, new_phys);
    }

    page_table_t* table = PAGE_TABLE_ADDR(PAGE_DIRECTORY_INDEX((uint32_t)virt));

    // clear the table if we have just created it
    if(new_table)
        memset(table, 0, sizeof(page_table_t));

    pte_t* pte = PAGE_TABLE_LOOKUP(table, virt);
    page_entry_add_attrib(pte, PTE_PRESENT | flags);
    page_entry_set_frame(pte, phys);

    vmmngr_flush_tlb_entry(virt);

    return ERR_MEM_SUCCESS;
}

void vmmngr_unmap(page_directory_t* page_directory, virtual_addr_t virt) {
    page_directory_t* virt_pd;
    if(page_directory == NULL) virt_pd = (page_directory_t*)VMMNGR_PD;
    else {
        map_temporary_pd(page_directory);
        virt_pd = (page_directory_t*)VMMNGR_RESERVED;
    }

    pde_t* pde = PAGE_DIRECTORY_LOOKUP(virt_pd, virt);
    if(!(*pde & PDE_PRESENT)) return;

    page_table_t* table = PAGE_TABLE_ADDR(PAGE_DIRECTORY_INDEX((uint32_t)virt));
    pte_t* pte = PAGE_TABLE_LOOKUP(table, virt);
    if(!(*pte & PTE_PRESENT)) return;

    physical_addr_t phys = (physical_addr_t)(*pte & PAGE_FRAME_BITS);
    pmmngr_free_block((void*)phys);
    // clear all attrib
    *pte = 0x0;

    vmmngr_flush_tlb_entry(virt);
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

    map_temporary_pd(pd);
    page_directory_t* virt_pd = (page_directory_t*)VMMNGR_RESERVED;

    // copy the current page directory
    page_directory_t* kernel_pd = (page_directory_t*)VMMNGR_PD;
    memcpy(virt_pd, kernel_pd, sizeof(page_directory_t));

    // set final entry to itself for recursive paging
    pde_t* pde = &virt_pd->entry[1023];
    page_entry_add_attrib(pde, PDE_PRESENT | PDE_WRITABLE);
    page_entry_set_frame(pde, (physical_addr_t)pd);

    // map itself to VMMNGR_PD
    vmmngr_map(pd, (physical_addr_t)pd, VMMNGR_PD, PTE_PRESENT | PTE_WRITABLE);

    return pd;
}

// free all memory lower than KERNEL_START of page_directory and itself
void vmmngr_free_page_directory(page_directory_t* page_directory) {
    map_temporary_pd(page_directory);
    page_directory_t* virt_pd = (page_directory_t*)VMMNGR_RESERVED;

    for(unsigned i = 0; i < 1024; i++) {
        // only free page table that map to address lower than KERNEL_START
        if(i * MMNGR_PAGE_SIZE * 1024 >= KERNEL_START) break;

        if(virt_pd->entry[i] == 0) continue;
        physical_addr_t phys_table = (virt_pd->entry[i] & PAGE_FRAME_BITS);
        page_table_t* virt_table = PAGE_TABLE_ADDR(i);

        // free memory to map that page table
        for(int j = 0; j < 1024; j++)
            pmmngr_free_block((void*)virt_table->entry[i]);

        // finally free the table
        pmmngr_free_block((void*)phys_table);
    }

    pmmngr_free_block(page_directory);
}

void vmmngr_switch_page_directory(page_directory_t* dir) {
    current_page_directory = dir;
    mapped_temporal_pd = 0;
    asm volatile("mov %0, %%cr3" : : "r" (dir));
}

void vmmngr_flush_tlb_entry(virtual_addr_t addr) {
	asm volatile("invlpg (%0)" : : "b" ((void*)addr) : "memory");
}

void vmmngr_init() {
    current_page_directory = kernel_page_directory;
}
