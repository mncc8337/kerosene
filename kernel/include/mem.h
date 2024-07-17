#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    ERR_MEM_OUT_OF_MEM,
    ERR_MEM_SUCCESS,
    ERR_MEM_FAILED,
    ERR_MEM_INVALID_DIR,
} MEM_ERR;

#define MMNGR_BLOCK_SIZE 4096
#define MMNGR_PAGE_SIZE 4096

#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3ff)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3ff)
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*(x) & ~0xfff)

typedef struct {
    uint32_t entry[1024] __attribute__((aligned(4096)));
} __attribute__((packed)) ptable;
typedef struct {
    uint32_t entry[1024] __attribute__((aligned(4096)));
} __attribute__((packed)) pdir;

enum PAGE_PTE_FLAGS {
    I86_PTE_PRESENT = 1,
    I86_PTE_WRITABLE = 2,
    I86_PTE_USER = 4,
    I86_PTE_WRITETHOUGH = 8,
    I86_PTE_NOT_CACHEABLE = 0x10,
    I86_PTE_ACCESSED = 0x20,
    I86_PTE_DIRTY = 0x40,
    I86_PTE_PAT = 0x80,
    I86_PTE_CPU_GLOBAL = 0x100,
    I86_PTE_LV4_GLOBAL = 0x200,
    I86_PTE_FRAME = 0x7ffff000
};

enum PAGE_PDE_FLAGS {
    I86_PDE_PRESENT = 1,
    I86_PDE_WRITABLE = 2,
    I86_PDE_USER = 4,
    I86_PDE_PWT = 8,
    I86_PDE_PCD = 0x10,
    I86_PDE_ACCESSED = 0x20,
    I86_PDE_DIRTY = 0x40,
    I86_PDE_4MB = 0x80,
    I86_PDE_CPU_GLOBAL = 0x100,
    I86_PDE_LV4_GLOBAL = 0x200,
    I86_PDE_FRAME = 0x7ffff000
};

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
void load_page_directory(uint32_t* pd);
void enable_paging();
uint32_t* vmmngr_ptable_lookup_entry(ptable* p, uint32_t addr);
uint32_t* vmmngr_pdirectory_lookup_entry(pdir* p, uint32_t addr);
MEM_ERR vmmngr_switch_pdirectory(pdir* dir);
pdir* vmmngr_get_directory();
void vmmngr_flush_tlb_entry(uint32_t addr);
MEM_ERR vmmngr_alloc_page(uint32_t* e);
void vmmngr_free_page(uint32_t* e);
MEM_ERR vmmngr_map_page(uint32_t phys, uint32_t virt);
MEM_ERR vmmngr_init();
