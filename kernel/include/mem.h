#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// 8 blocks per byte
#define PMMNGR_BLOCKS_PER_BYTE 8
// block size (4k)
#define PMMNGR_BLOCK_SIZE 4096
// block alignment
#define PMMNGR_BLOCK_ALIGN PMMNGR_BLOCK_SIZE

typedef struct {
	uint32_t base_low;   uint32_t base_high;
	uint32_t length_low; uint32_t length_high;
	uint32_t type;
	uint32_t acpi;
 
} __attribute__((packed)) memmap_entry_t;

// still unsure which one is correct so i left another one here
// typedef struct {
// 	uint32_t base_low;
// 	uint32_t base_high;
// 	uint32_t length_low;
// 	uint32_t length_high;
// 	uint16_t type;
// 	uint16_t acpi;
// 	uint32_t padding;
//
// } __attribute__((packed)) memmap_entry_t;

enum MEMMAP_ENTRY_REGION_TYPE {
    MMER_TYPE_UNKNOWN,
    MMER_TYPE_USABLE,
    MMER_TYPE_RESERVED,
    MMER_TYPE_ACPI,
    MMER_TYPE_ACPI_NVS,
    MMER_TYPE_BADMEM,
    
};

// pmmngr.c
uint32_t pmmngr_get_block_count();
uint32_t pmmngr_get_free_block_count();
void pmmngr_mmap_set(int bit);
void pmmngr_mmap_unset(int bit);
bool pmmngr_mmap_test(int bit);
int pmmngr_mmap_first_free();
void pmmngr_init_region(uint32_t base, size_t size);
void pmmngr_deinit_region(uint32_t base, size_t size);
void* pmmngr_alloc_block();
void pmmngr_free_block (uint32_t addr);
void pmmngr_init(size_t msize, uint32_t* bitmap);
