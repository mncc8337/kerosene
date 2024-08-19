#pragma once

#include "data_structure/ordered_array.h"

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
typedef uint64_t virtual_addr_t;

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
    pte_t entry[1024] __attribute__((aligned(4096)));
} __attribute__((packed)) ptable;
typedef struct {
    pde_t entry[1024] __attribute__((aligned(4096)));
} __attribute__((packed)) pdir;

typedef struct {
    ordered_array_t array;
    virtual_addr_t start_address;
    virtual_addr_t end_address;
    virtual_addr_t max_address;
    bool supervisor;
    bool readonly;
} heap_t;

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
void pmmngr_init(physical_addr_t base, size_t size);

// vmmngr_pte.c
void pte_add_attrib(pte_t* e, uint32_t attrib);
void pte_del_attrib(pte_t* e, uint32_t attrib);
void pte_set_frame(pte_t* e, physical_addr_t addr);
bool pte_is_present(pte_t e);
bool pte_is_writable(pte_t e);
uint32_t pte_pfn(pte_t e);

// vmmngr_pde.c
void pde_add_attrib(pde_t* e, uint32_t attrib);
void pde_del_attrib(pde_t* e, uint32_t attrib);
void pde_set_frame(pde_t* e, physical_addr_t addr);
bool pde_is_present(pde_t e);
bool pde_is_user(pde_t e);
bool pde_is_4mb(pde_t e);
bool pde_is_writable(pde_t e);
uint32_t pde_pfn(pde_t e);
void pde_enable_global(pde_t e);

// vmmngr.c
uint32_t* vmmngr_ptable_lookup_entry(ptable* p, virtual_addr_t addr);
uint32_t* vmmngr_pdirectory_lookup_entry(pdir* p, virtual_addr_t addr);
MEM_ERR vmmngr_switch_pdirectory(pdir* dir);
pdir* vmmngr_get_directory();
void vmmngr_flush_tlb_entry(virtual_addr_t addr);
MEM_ERR vmmngr_alloc_page(pte_t* e);
void vmmngr_free_page(pte_t* e);
MEM_ERR vmmngr_map_page(physical_addr_t phys, virtual_addr_t virt);
MEM_ERR vmmngr_init();

// heap.c
void* heap_malloc(heap_t* heap, size_t byte);
void heap_free(heap_t* heap, void* ptr);
heap_t* heap_create(virtual_addr_t start, virtual_addr_t end, size_t size, bool supervisor, bool readonly);
