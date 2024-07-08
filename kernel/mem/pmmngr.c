#include "mem.h"

static uint32_t* mem_addr;
static size_t mem_size;
static uint32_t used_block;
static uint32_t total_block;

static void set_bit(uint32_t bit) {
    mem_addr[bit/32] |= (1 << (bit % 32));
}
static void unset_bit(uint32_t bit) {
    mem_addr[bit/32] |= ~(1 << (bit % 32));
}
// static bool test_bit(uint32_t bit) {
//     return mem_addr[bit/32] & (1 << (bit % 32));
// }

static int find_first_free() {
    for(unsigned int i = 0; i < total_block/32; i++) {
        if(mem_addr[i] == 0xfffffff) continue;
        for(int j = 0; j < 32;j++) {
            int bit = 1 << j;
            if(!(mem_addr[i] & bit))
                return i * 32 + j;
        }
    }
    return -1;
}

size_t pmmngr_get_size() {
    return total_block * MMNGR_BLOCK_SIZE;
}
size_t pmmngr_get_used_size() {
    return used_block * MMNGR_BLOCK_SIZE;
}
size_t pmmngr_get_free_size() {
    return (total_block - used_block) * MMNGR_BLOCK_SIZE;
}

void pmmngr_init_region(uint32_t base, size_t size) {
    //round base up to 4k align
    if(base & MMNGR_BLOCK_SIZE) {
        base += MMNGR_BLOCK_SIZE;
        base &= ~MMNGR_BLOCK_SIZE;
    }
    if(size & MMNGR_BLOCK_SIZE) {
        size += MMNGR_BLOCK_SIZE;
        size &= ~MMNGR_BLOCK_SIZE;
    }

    uint32_t start = base / MMNGR_BLOCK_SIZE;
    uint32_t block = size / MMNGR_BLOCK_SIZE;

    for (; block > 0; block--) {
        unset_bit(start++);
        used_block--;
    }

    set_bit(0); 
}
void pmmngr_deinit_region(uint32_t base, size_t size) {
    //round base up to 4k align
    if(base & MMNGR_BLOCK_SIZE) {
        base += MMNGR_BLOCK_SIZE;
        base &= ~MMNGR_BLOCK_SIZE;
    }
    if(size & MMNGR_BLOCK_SIZE) {
        size += MMNGR_BLOCK_SIZE;
        size &= ~MMNGR_BLOCK_SIZE;
    }
    uint32_t start = base / MMNGR_BLOCK_SIZE;
    uint32_t block = size / MMNGR_BLOCK_SIZE;

    for (;block > 0; block--) {
        set_bit(start++);
        used_block++;
    }
}

void* pmmngr_alloc_block() {
    if(total_block - used_block == 0) return 0;

    int frame = find_first_free();
    if(frame == -1) return 0;

    set_bit(frame);

    uint32_t base = frame * MMNGR_BLOCK_SIZE;
    used_block++;

    return (void*)base;
}
void pmmngr_free_block(void* base) {
    uint32_t addr = (uint32_t)base;
    int frame = addr / MMNGR_BLOCK_SIZE;

    if(frame == 0) return;

    unset_bit(frame);
    used_block--;
}

void pmmngr_init(uint32_t base, size_t size) {
    mem_addr = (uint32_t*)base;
    mem_size = size;
    total_block = mem_size / MMNGR_BLOCK_SIZE;

    // assume that all memory are in use
    used_block = total_block;
    for(unsigned int i = 0; i < total_block / 32; i++)
        mem_addr[i] = 0xffffffff;
}
