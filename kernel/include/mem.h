#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MMNGR_BLOCK_SIZE 4096
#define MMNGR_PAGE_SIZE 4096

#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3ff)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3ff)
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*(x) & ~0xfff)

typedef struct {
    uint32_t base_low;   uint32_t base_high;
    uint32_t length_low; uint32_t length_high;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) memmap_entry_t;

typedef struct {
    uint32_t entry[1024] __attribute__((aligned(4096)));
} __attribute__((packed)) ptable;
typedef struct {
    uint32_t entry[1024] __attribute__((aligned(4096)));
} __attribute__((packed)) pdir;


enum MEMMAP_ENTRY_REGION_TYPE {
    MMER_TYPE_UNKNOWN,
    MMER_TYPE_USABLE,
    MMER_TYPE_RESERVED,
    MMER_TYPE_ACPI,
    MMER_TYPE_ACPI_NVS,
    MMER_TYPE_BADMEM,
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

// vmmngr.c
int vmmngr_init();
