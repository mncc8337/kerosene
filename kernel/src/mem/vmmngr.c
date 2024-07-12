#include "mem.h"
#include "errorcode.h"

extern void load_page_directory(uint32_t*);
extern void enable_paging();

int vmmngr_init() {
    pdir* page_directory = (pdir*)pmmngr_alloc_block();
    if(!page_directory) return ERR_OUT_OF_MEM;

    for(unsigned int i = 0; i < 1024; i++)
        page_directory->entry[i] = 0x00000002;

    ptable* page_table = (ptable*)pmmngr_alloc_block();
    if(!page_directory) return ERR_OUT_OF_MEM;

    for(unsigned int i = 0; i < 1024; i++)
        page_table->entry[i] = (i * MMNGR_PAGE_SIZE) | 3;

    page_directory->entry[0] = ((uint32_t)page_table) | 3;

    load_page_directory((uint32_t*)page_directory);
    enable_paging();

    return ERR_SUCCESS;
}
