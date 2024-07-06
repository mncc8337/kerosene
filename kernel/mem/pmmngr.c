#include "mem.h"

// a physical memory manager that allocate
// memory with dynamic size block
// block info are stored in the array block (below)

static mem_block_t block[1024];
static size_t block_cnt = 0;
static size_t total_size = 0;
static size_t used_size = 0;

uint32_t pmmngr_get_free_size() {
    return total_size - used_size;
}
uint32_t pmmngr_get_used_size() {
    return used_size;
}

static unsigned int find_block_with_base(void* ptr) {
    // binary search because the array is sorted
    unsigned int block_id = 0;
    unsigned int l = 1;
    unsigned int r = block_cnt-1;
    while(l < r) {
        block_id = (l + r)/2;
        if(block[block_id].base == (uint32_t)ptr) break;
        if(block[block_id].base < (uint32_t)ptr) l = block_id;
        else r = block_id;
    }

    return block_id;
}

// merge 2 adjacent blocks: block_id and block_id+1
static void merge_block(unsigned int block_id) {
    if(block_id+1 < block_cnt
            && !block[block_id+1].used
            && block[block_id].base + block[block_id].size == block[block_id+1].base
    ) {
        block[block_id].size += block[block_id+1].size;

        // shift the other blocks to the left
        for(unsigned int i = block_id+2; i < block_cnt; i++)
            block[i-1] = block[i];
        block_cnt--;
    }
}
static void split_block(unsigned int block_id, size_t first_block_size) {
    // split the block into 2
    mem_block_t splitted;
    splitted.used = false;
    splitted.base = block[block_id].base + first_block_size;
    splitted.size = block[block_id].size - first_block_size;
    // set the original block
    block[block_id].size = first_block_size;

    // shift the other blocks to the right
    for(unsigned int i = block_cnt-1; i > block_id; i--)
        block[i+1] = block[i];
    block_cnt++;
    // append the new block
    block[block_id+1] = splitted;
}

void* pmmngr_malloc(size_t byte) {
    // out of mem
    if(used_size + byte > total_size) return 0;

    unsigned int block_id = 0;
    for(unsigned int i = 1; i < block_cnt; i++) {
        if(block[i].used) continue;
        if(block[i].size < byte) continue;
        block_id = i;
        break;
    }
    if(block_id == 0) return 0;

    block[block_id].used = true;
    used_size += byte;

    if(block[block_id].size > byte)
        split_block(block_id, byte);

    return (void*)(block[block_id].base);
}

void pmmngr_free(void* ptr) {
    // do not free the 0x0
    if((uint32_t)ptr == 0) return;

    unsigned int block_id = find_block_with_base(ptr); 
    if(block_id == 0 || !block[block_id].used) return;

    block[block_id].used = false;
    used_size -= block[block_id].size;

    // merge with the next block
    merge_block(block_id);
    // merge with the prev block
    merge_block(block_id-1);
}

bool pmmngr_extend_block(void* ptr, size_t ammount) {
    if((int)ptr == 0) return false;

    unsigned int block_id = find_block_with_base(ptr);
    if(block_id == 0
            || block_id+1 >= block_cnt
            || block[block_id+1].used
            || block[block_id+1].size < ammount
            || block[block_id].base + block[block_id].size != block[block_id+1].base)
        return false;

    block[block_id].size += ammount;
    block[block_id+1].size -= ammount;

    if(block[block_id].used)
        used_size += ammount;

    if(block[block_id].size == 0)
        merge_block(block_id);

    return true;
}

// once lock a region cannot be unlocked or used
void pmmngr_remove_region(void* base, size_t size) {
    uint32_t addr = (uint32_t)base;

    unsigned int block_id = 0;
    for(block_id = 1; block_id < block_cnt; block_id++) {
        if(addr >= block[block_id].base
                && addr < block[block_id].base + block[block_id].size)
            break;
    }

    if(addr > block[block_id].base) {
        split_block(block_id, addr - block[block_id].base);
        block_id++;
    }
    bool additional_block = false;
    if(size > block[block_id].size) {
        additional_block = true;
        split_block(block_id+1, size - block[block_id].size);
    }

    // update total size
    total_size -= block[block_id].size;
    if(additional_block)
        total_size -= block[block_id+1].size;

    // remove region
    for(unsigned int i = block_id + 1 + additional_block; i < block_cnt; i++)
        block[i-1-additional_block] = block[i];
    block_cnt -= 1 + additional_block;
}

void pmmngr_init(memmap_entry_t* mmptr, size_t entry_cnt) {
    // add a block with address 0x0 and set it to used
    // so that it wont be touched
    // and we can use 0x0 as error code
    // to return in malloc
    block[0].used = true;
    block[0].size = 1;
    block_cnt = 1;

    for(unsigned int i = 0; i < entry_cnt; i++) {
        // if not usable or not apci reclaimable then skip
        if(mmptr[i].type != MMER_TYPE_USABLE && mmptr[i].type != MMER_TYPE_ACPI)
            continue;

        // ignore memory higher than 4GiB
        // the map is sorted so we can break to ignore all data next to it
        if(mmptr[i].base_high > 0 || mmptr[i].length_high > 0) break;

        // avoid address 0x0 as explained above
        if(mmptr[i].base_low == 0x0) {
            mmptr[i].base_low = 0x1;
            mmptr[i].length_low -= 1;
        }

        total_size += mmptr[i].length_low;

        block[block_cnt].used = false;
        block[block_cnt].base = mmptr[i].base_low;
        block[block_cnt].size = mmptr[i].length_low;
        block_cnt++;
    }
}
