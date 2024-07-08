#include "mem.h"

// FIXME:
// idk why these table make virtualbox raise Invalid Opcode
uint32_t ptable[1024] __attribute__((aligned(MMNGR_PAGE_SIZE)));
uint32_t page_directory[1024] __attribute__((aligned(MMNGR_PAGE_SIZE)));

extern void load_page_directory(uint32_t*);
extern void enable_paging();

void vmmngr_init() {
    // lock the mem region of these table
    pmmngr_deinit_region((uint32_t)ptable, MMNGR_PAGE_SIZE * 1024);
    pmmngr_deinit_region((uint32_t)page_directory, MMNGR_PAGE_SIZE * 1024);

    for(int i = 0; i < 1024; i++)
        page_directory[i] = 0x00000002;
    for (unsigned int i = 0; i < 1024; i++)
        ptable[i] = (i * MMNGR_PAGE_SIZE) | 3;

    page_directory[0] = ((uint32_t)ptable) | 3;

    load_page_directory(page_directory);
    enable_paging();
}
