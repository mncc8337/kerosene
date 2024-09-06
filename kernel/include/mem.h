#pragma once

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

typedef enum {
    ERR_MEM_SUCCESS,
    ERR_MEM_OOM,
    ERR_MEM_INVALID_DIR,
    ERR_MEM_UNMAPPED
} MEM_ERR;

#define MMNGR_PAGE_SIZE 4096

typedef uint32_t pde_t;
typedef uint32_t pte_t;
typedef uint32_t physical_addr_t;
typedef uint32_t virtual_addr_t;

#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3ff)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3ff)

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

#define HEAP_SUPERVISOR 0b01
#define HEAP_READONLY   0b10

#define HEAP_FREE 0x7ea9f2ee
#define HEAP_USED 0x7ea942ed

#define HEAP_NEXT_HEADER(header) (heap_header_t*)((void*)(header) + sizeof(heap_header_t) + header->size)
#define HEAP_FIRST_HEADER(heap) (heap_header_t*)((void*)(heap) + sizeof(heap_t))

typedef struct {
    uint32_t end;
    uint32_t max_addr;
    uint32_t min_size;
    uint8_t flags;
} heap_t;

typedef struct heap_header {
    uint32_t magic;
    uint32_t size;
    struct heap_header* prev;
    // uint32_t caller;
} __attribute__((packed)) heap_header_t;

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
uint32_t* vmmngr_page_table_lookup_entry(page_table_t* p, virtual_addr_t addr);
uint32_t* vmmngr_page_directory_lookup_entry(page_directory_t* p, virtual_addr_t addr);
page_directory_t* vmmngr_get_directory();
MEM_ERR vmmngr_alloc_page(pte_t* pte);
void vmmngr_free_page(pte_t* pte);
MEM_ERR vmmngr_map_page(physical_addr_t phys, virtual_addr_t virt, unsigned flags);
MEM_ERR vmmngr_unmap_page(virtual_addr_t virt);
physical_addr_t vmmngr_to_physical_addr(virtual_addr_t virt);
MEM_ERR vmmngr_switch_pdirectory(page_directory_t* dir);
void vmmngr_load_page_directory(physical_addr_t addr);
void vmmngr_flush_tlb_entry(virtual_addr_t addr);
MEM_ERR vmmngr_init();

// heap.c
heap_t* heap_new(uint32_t start, uint32_t size, size_t max_size, uint8_t flags);
bool heap_expand(heap_t* heap, size_t page_count, heap_header_t* last_header);
void heap_contract(heap_t* heap, size_t page_count, heap_header_t* last_header);
void* heap_alloc(heap_t* heap, size_t size, bool page_align);
void heap_free(heap_t* heap, void* addr);

// kheap.c
bool kheap_init();
void* kmalloc(size_t size);
void kfree(void* addr);
