#include "mem.h"
#include "string.h"

extern void load_page_directory(uint32_t*);
extern void enable_paging();

static pdir* current_directory = 0;
static uint32_t current_pdbr = 0;

uint32_t* vmmngr_ptable_lookup_entry(ptable* p, uint32_t addr) {
    if(p) return &p->entry[PAGE_TABLE_INDEX(addr)];
    return (uint32_t*)ERR_MEM_FAILED;
}

uint32_t* vmmngr_pdirectory_lookup_entry(pdir* p, uint32_t addr) {
    if(p) return &p->entry[PAGE_TABLE_INDEX(addr)];
    return (uint32_t*)ERR_MEM_FAILED;
}

MEM_ERR vmmngr_switch_pdirectory(pdir* dir) {
    if(!dir) return ERR_MEM_INVALID_DIR;

    current_directory = dir;
    load_page_directory((uint32_t*)current_pdbr);
    return ERR_MEM_SUCCESS;
}

pdir* vmmngr_get_directory() {
    return current_directory;
}

void vmmngr_flush_tlb_entry(uint32_t addr) {
	asm volatile("invlpg (%0)" : : "r" (addr) : "memory");
}

MEM_ERR vmmngr_alloc_page(uint32_t* e) {
    void* p = pmmngr_alloc_block();
    if(!p) return ERR_MEM_OUT_OF_MEM;

    pte_set_frame(e, (uint32_t)p);
    pte_add_attrib(e, I86_PTE_PRESENT);

    return ERR_MEM_SUCCESS;
}

void vmmngr_free_page(uint32_t* e) {
    void* p = (void*)pte_pfn(*e);
    if(p) pmmngr_free_block(p);

    pte_del_attrib(e, I86_PTE_PRESENT);
}

MEM_ERR vmmngr_map_page(uint32_t phys, uint32_t virt) {
    // get page directory
    pdir* pageDirectory = vmmngr_get_directory();

    // get page table
    uint32_t* e = &pageDirectory->entry[PAGE_DIRECTORY_INDEX(virt)];
    if((*e & I86_PTE_PRESENT) != I86_PTE_PRESENT) {
        // if page table not present then allocate it
        ptable* table = (ptable*)pmmngr_alloc_block();
        if(!table) return ERR_MEM_OUT_OF_MEM;

        // clear page table
        memset(table, 0, sizeof(ptable));

        // create a new entry
        uint32_t* entry = &pageDirectory->entry[PAGE_DIRECTORY_INDEX(virt)];
        pde_add_attrib(entry, I86_PDE_PRESENT);
        pde_add_attrib(entry, I86_PDE_WRITABLE);
        pde_set_frame(entry, (uint32_t)table);
   }

    // get table
    ptable* table = (ptable*)PAGE_GET_PHYSICAL_ADDRESS(e);

    // get page
    uint32_t* page = &table->entry[PAGE_TABLE_INDEX(virt)];
    pte_set_frame(page, phys);
    pte_add_attrib(page, I86_PTE_PRESENT);

    return ERR_MEM_SUCCESS;
}

MEM_ERR vmmngr_init() {
    ptable* ptable1 = (ptable*)pmmngr_alloc_block();
    if(!ptable1) return ERR_MEM_OUT_OF_MEM;

    ptable* ptable2 = (ptable*)pmmngr_alloc_block();
    if(!ptable2) return ERR_MEM_OUT_OF_MEM;

    int frame = 0;
    int virt = 0;
    for(int i = 0; i < 1024; i++) {
        frame += MMNGR_PAGE_SIZE;
        virt += MMNGR_PAGE_SIZE;
        uint32_t page = 0;
        pte_add_attrib(&page, I86_PTE_PRESENT);
        pte_set_frame(&page, frame);
        ptable1->entry[PAGE_TABLE_INDEX(virt)] = page;
    }

    frame = 0x100000;
    virt = 0xc0000000;
    for(int i = 0; i < 1024; i++) {
        frame += MMNGR_PAGE_SIZE;
        virt += MMNGR_PAGE_SIZE;
        uint32_t page = 0;
        pte_add_attrib(&page, I86_PTE_PRESENT);
        pte_set_frame(&page, frame);
        ptable2->entry[PAGE_TABLE_INDEX(virt)] = page;
    }

    pdir* page_directory = (pdir*)pmmngr_alloc_multi_block(3);
    if(!page_directory) return ERR_MEM_OUT_OF_MEM;

    for(int i = 0; i < 1024; i++)
        page_directory->entry[i] = 0x00000002;

    page_directory->entry[PAGE_DIRECTORY_INDEX(0x00000000)] = ((uint32_t)ptable1) | 3;
    page_directory->entry[PAGE_DIRECTORY_INDEX(0xc0000000)] = ((uint32_t)ptable2) | 3;

    current_pdbr = (uint32_t)page_directory->entry;
    vmmngr_switch_pdirectory(page_directory);
    enable_paging();

    return ERR_MEM_SUCCESS;
}
