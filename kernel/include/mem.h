#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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

typedef struct {
    bool used;
    uint32_t base;
    uint32_t size;
} mem_block_t;

// pmmngr.c
uint32_t pmmngr_get_free_size();
uint32_t pmmngr_get_used_size();
void* pmmngr_alloc(size_t byte);
void pmmngr_free(void* ptr);
bool pmmngr_extend_block(void* ptr, size_t ammount);
void pmmngr_remove_region(void* base, size_t size);
void pmmngr_init(memmap_entry_t* mmptr, size_t entry_cnt);
