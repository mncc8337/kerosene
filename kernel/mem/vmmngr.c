#include "mem.h"

extern void load_page_directory(uint32_t*);
extern void enable_paging();

void vmmngr_init() {
    // commented because qemu will freeze if i do so
    pdir* page_directory /* = (pdir*)pmmngr_alloc_multi_block(3) */;
    ptable* page_table /* = (ptable*)pmmngr_alloc_block() */;

    for(int i = 0; i < 1024; i++)
        page_directory->entry[i] = 0x00000002;
    for(unsigned int i = 0; i < 1024; i++)
        page_table->entry[i] = (i * MMNGR_PAGE_SIZE) | 3;

    page_directory->entry[0] = ((uint32_t)page_table) | 3;

    load_page_directory((uint32_t*)page_directory);
    enable_paging();
}
