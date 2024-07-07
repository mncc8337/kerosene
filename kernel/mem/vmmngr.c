#include "mem.h"

uint32_t ptable[1024] __attribute__((aligned(MMNGR_PAGE_SIZE)));
uint32_t page_directory[1024] __attribute__((aligned(MMNGR_PAGE_SIZE)));

extern void load_page_directory(uint32_t*);
extern void enable_paging();

void vmmngr_init() {
    // lock the mem region of these table
    pmmngr_remove_region(ptable, MMNGR_PAGE_SIZE * 1024);
    pmmngr_remove_region(page_directory, MMNGR_PAGE_SIZE * 1024);

    for(int i = 0; i < 1024; i++)
        page_directory[i] = 0x00000002;
    for (unsigned int i = 0; i < 1024; i++)
        ptable[i] = (i * 0x1000) | 3;

    page_directory[0] = ((uint32_t)ptable) | 3;

    load_page_directory(page_directory);
    enable_paging();
}
