#pragma once

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

typedef enum {
    ERR_MEM_FAILED,
    ERR_MEM_SUCCESS,
    ERR_MEM_INVALID_DIR,
} MEM_ERR;

#ifndef NULL
#define NULL
#endif

#define MMNGR_BLOCK_SIZE 4096
#define MMNGR_PAGE_SIZE 4096

#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3ff)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3ff)
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*(x) & ~0xfff)

#define PTE_PRESENT       1
#define PTE_WRITABLE      2
#define PTE_USER          4
#define PTE_WRITETHOUGH   8
#define PTE_NOT_CACHEABLE 0x10
#define PTE_ACCESSED      0x20
#define PTE_DIRTY         0x40
#define PTE_PAT           0x80
#define PTE_CPU_GLOBAL    0x100
#define PTE_LV4_GLOBAL    0x200
#define PTE_FRAME         0x7ffff000

#define PDE_PRESENT    1
#define PDE_WRITABLE   2
#define PDE_USER       4
#define PDE_PWT        8
#define PDE_PCD        0x10
#define PDE_ACCESSED   0x20
#define PDE_DIRTY      0x40
#define PDE_4MB        0x80
#define PDE_CPU_GLOBAL 0x100
#define PDE_LV4_GLOBAL 0x200
#define PDE_FRAME      0x7ffff000

typedef struct {
    uint32_t entry[1024] __attribute__((aligned(4096)));
} __attribute__((packed)) ptable;
typedef struct {
    uint32_t entry[1024] __attribute__((aligned(4096)));
} __attribute__((packed)) pdir;

// pmmngr.c
void pmmngr_update_usage();
size_t pmmngr_get_size();
size_t pmmngr_get_used_size();
size_t pmmngr_get_free_size();
void pmmngr_init_region(uint32_t base, size_t size);
void pmmngr_deinit_region(uint32_t base, size_t size);
void* pmmngr_alloc_block();
void* pmmngr_alloc_multi_block(size_t cnt);
void pmmngr_free_block(void* base);
void pmmngr_free_multi_block(void* base, size_t cnt);
void pmmngr_init(uint32_t base, size_t size);

// vmmngr_pte.c
void pte_add_attrib(uint32_t* e, uint32_t attrib);
void pte_del_attrib(uint32_t* e, uint32_t attrib);
void pte_set_frame(uint32_t* e, uint32_t addr);
bool pte_is_present(uint32_t e);
bool pte_is_writable(uint32_t e);
uint32_t pte_pfn(uint32_t e);

// vmmngr_pde.c
void pde_add_attrib(uint32_t* e, uint32_t attrib);
void pde_del_attrib(uint32_t* e, uint32_t attrib);
void pde_set_frame(uint32_t* e, uint32_t addr);
bool pde_is_present(uint32_t e);
bool pde_is_user(uint32_t e);
bool pde_is_4mb(uint32_t e);
bool pde_is_writable(uint32_t e);
uint32_t pde_pfn(uint32_t e);
void pde_enable_global(uint32_t e);

// vmmngr.c
uint32_t* vmmngr_ptable_lookup_entry(ptable* p, uint64_t addr);
uint32_t* vmmngr_pdirectory_lookup_entry(pdir* p, uint64_t addr);
MEM_ERR vmmngr_switch_pdirectory(pdir* dir);
pdir* vmmngr_get_directory();
void vmmngr_flush_tlb_entry(uint32_t addr);
MEM_ERR vmmngr_alloc_page(uint32_t* e);
void vmmngr_free_page(uint32_t* e);
MEM_ERR vmmngr_map_page(uint32_t phys, uint64_t virt);
MEM_ERR vmmngr_init();

// heap.c
void* kmalloc(int byte);
void kfree(void* ptr);
