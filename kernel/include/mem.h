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

#define PAGE_DIRECTORY_INDEX(addr) (((addr) >> 22) & 0x3ff)
#define PAGE_TABLE_INDEX(addr) (((addr) >> 12) & 0x3ff)
#define PAGE_TABLE_LOOKUP(page, addr) (&(page)->entry[PAGE_TABLE_INDEX(addr)])
#define PAGE_DIRECTORY_LOOKUP(page, addr) (&(page)->entry[PAGE_DIRECTORY_INDEX(addr)])

#define HEAP_SUPERVISOR 0b01
#define HEAP_READONLY   0b10

#define HEAP_FREE 0x7ea9f2ee
#define HEAP_USED 0x7ea942ed

#define HEAP_NEXT_HEADER(header) (heap_header_t*)((void*)(header) + sizeof(heap_header_t) + header->size)
#define HEAP_FIRST_HEADER(heap) (heap_header_t*)((void*)(heap) + sizeof(heap_t))

typedef struct {
    pte_t entry[1024] __attribute__((aligned(4096)));
} __attribute__((packed)) page_table_t;

typedef struct {
    pde_t entry[1024] __attribute__((aligned(4096)));
} __attribute__((packed)) page_directory_t;

typedef struct {
    uint32_t end;
    uint32_t max_addr;
    uint32_t min_size;
    uint8_t flags;
} __attribute__((packed)) heap_t;

typedef struct heap_header {
    uint32_t magic;
    uint32_t size;
    struct heap_header* prev;
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
page_directory_t* vmmngr_get_directory();
physical_addr_t vmmngr_to_physical_addr(page_directory_t* pd, virtual_addr_t virt);
MEM_ERR vmmngr_map(page_directory_t* page_directory, physical_addr_t phys, virtual_addr_t virt, unsigned flags);
MEM_ERR vmmngr_unmap(page_directory_t* page_directory, virtual_addr_t virt);
MEM_ERR vmmngr_alloc_page(pte_t* pte);
void vmmngr_free_page(pte_t* pte);
page_directory_t* vmmngr_alloc_page_directory();
MEM_ERR vmmngr_switch_page_directory(page_directory_t* dir);
void vmmngr_flush_tlb_entry(virtual_addr_t addr);
void vmmngr_init();

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
