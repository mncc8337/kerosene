#include "mem.h"

// size of physical memory
uint32_t memsize = 0;
// number of blocks currently in use
uint32_t used_blocks = 0;
// maximum number of available memory blocks
uint32_t max_blocks = 0;
// memory map bit array. Each bit represents a memory block
uint32_t* memmap = 0;

uint32_t pmmngr_get_block_count() {
    return max_blocks;
}
uint32_t pmmngr_get_free_block_count() {
    return max_blocks - used_blocks;
}

void pmmngr_mmap_set(int bit) {
    memmap[bit / 32] |= (1 << (bit % 32));
}
void pmmngr_mmap_unset(int bit) {
    memmap[bit / 32] &= ~(1 << (bit % 32));
}
bool pmmngr_mmap_test(int bit) {
    return memmap[bit / 32] &  (1 << (bit % 32));
}
int pmmngr_mmap_first_free() {
    // find the first free bit
    for(uint32_t i = 0; i< pmmngr_get_block_count() / 32; i++) {
        if(memmap[i] != 0xffffffff) {
            for(int j = 0; j < 32; j++) {
                int bit = 1 << j;
                if(!(memmap[i] & bit)) return i * 32 + j;
            }
        }
    }
    return -1;
}

void pmmngr_init_region(uint32_t base, size_t size) {
    // round up to 4k align
    if(base & 0xfff) {
        base += 4096;
        base &= ~0xfff;
    }
    if(size & 0xfff) {
        size += 4096;
        size &= ~0xfff;
    }

    uint32_t start = base / PMMNGR_BLOCK_SIZE;
    uint32_t blocks = size / PMMNGR_BLOCK_SIZE;

    for(; blocks > 0; blocks--) {
        pmmngr_mmap_unset(start++);
        used_blocks--;
    }

    pmmngr_mmap_set(0); // first block is always set
}
void pmmngr_deinit_region(uint32_t base, size_t size) {
    // round up to 4k align
    if(base & 0xfff) {
        base += 4096;
        base &= ~0xfff;
    }
    if(size & 0xfff) {
        size += 4096;
        size &= ~0xfff;
    }

    int start = base / PMMNGR_BLOCK_SIZE;
    int blocks = size / PMMNGR_BLOCK_SIZE;

    for(; blocks > 0; blocks--) {
        pmmngr_mmap_set(start++);
        used_blocks++;
    }
}

void* pmmngr_alloc_block() {
    if(pmmngr_get_free_block_count() <= 0)
        return 0; //out of memory

    int frame =pmmngr_mmap_first_free();

    if(frame == -1)
        return 0; //out of memory

    pmmngr_mmap_set(frame);

    void* addr = (void*)(frame * PMMNGR_BLOCK_SIZE);
    used_blocks++;

    return addr;
}
void pmmngr_free_block(uint32_t addr) {
    int frame = addr / PMMNGR_BLOCK_SIZE;
    pmmngr_mmap_unset(frame);
    used_blocks--;
}

void pmmngr_init(size_t msize, uint32_t* bitmap) {
    memsize = msize;
    memmap  = bitmap;
    max_blocks  = memsize / PMMNGR_BLOCK_SIZE;
    used_blocks = max_blocks;

    // by default all memory is in use to avoid errors
    for(int i = 0; i < max_blocks / PMMNGR_BLOCKS_PER_BYTE; i++)
        memmap[i] = 0xf;
}
