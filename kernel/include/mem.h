#pragma once

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

typedef enum {
    ERR_MEM_OOM,
    ERR_MEM_SUCCESS,
    ERR_MEM_INVALID_DIR,
} MEM_ERR;

#define MMNGR_BLOCK_SIZE 4096
#define MMNGR_PAGE_SIZE 4096

typedef uint32_t pde_t;
typedef uint32_t pte_t;
typedef uint32_t physical_addr_t;
typedef uint32_t virtual_addr_t;

#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3ff)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3ff)
// #define PAGE_GET_PHYSICAL_ADDRESS(x) (*(x) & ~0xfff)

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

typedef struct {
    pte_t entry[1024] __attribute__((aligned(4096)));
} __attribute__((packed)) page_table_t;
typedef struct {
    pde_t entry[1024] __attribute__((aligned(4096)));
} __attribute__((packed)) page_directory_t;

// pmmngr.c
void pmmngr_update_usage();
size_t pmmngr_get_size();
size_t pmmngr_get_used_size();
size_t pmmngr_get_free_size();
void pmmngr_init_region(physical_addr_t base, size_t size);
void pmmngr_deinit_region(physical_addr_t base, size_t size);
void* pmmngr_alloc_block();
void* pmmngr_alloc_multi_block(size_t cnt);
void pmmngr_free_block(void* base);
void pmmngr_free_multi_block(void* base, size_t cnt);
void pmmngr_init(size_t size);

// vmmngr.c
uint32_t* vmmngr_ptable_lookup_entry(page_table_t* p, virtual_addr_t addr);
uint32_t* vmmngr_pdirectory_lookup_entry(page_directory_t* p, virtual_addr_t addr);
page_directory_t* vmmngr_get_directory();
MEM_ERR vmmngr_alloc_page(pte_t* pte);
void vmmngr_free_page(pte_t* pte);
MEM_ERR vmmngr_map_page(physical_addr_t phys, virtual_addr_t virt, unsigned flags);
MEM_ERR vmmngr_switch_pdirectory(page_directory_t* dir);
void vmmngr_load_page_directory(physical_addr_t addr);
void vmmngr_flush_tlb_entry(virtual_addr_t addr);
MEM_ERR vmmngr_init();
