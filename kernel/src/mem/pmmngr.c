#include <mem.h>

#include <string.h>

// bitmap, the length is hardcoded because memory size will not exceed 3GiB
static uint32_t bitmap[24576]; // 3GiB * 1024*1024*1024 / 4096(block size) / 32 (size of uint32_t)
static size_t used_block;
static size_t total_block;

static void set_bit(uint32_t bit) {
    bitmap[bit/32] |= (1 << (bit % 32));
}
static void unset_bit(uint32_t bit) {
    bitmap[bit/32] &= ~(1 << (bit % 32));
}
static bool test_bit(uint32_t bit) {
    return (bitmap[bit/32] >> (bit % 32)) & 1;
}

static int find_first_free_block() {
    for(unsigned i = 0; i < total_block/32; i++) {
        if(bitmap[i] == 0xffffffff) continue;
        for(int j = 0; j < 32; j++) {
            unsigned bit_index = i * 32 + j;
            // stop checking if we're past the end of RAM
            if(bit_index >= total_block)
                return -1;
            
            int bit = 1 << j;
            if(!(bitmap[i] & bit)) {
                return bit_index;
            }
        }
    }
    return -1;
}
static int find_first_free(size_t frame_cnt) {
    if(frame_cnt == 0) return -1;
    if(frame_cnt == 1) return find_first_free_block();

    if(frame_cnt > total_block)
        return -1;

    for(uint32_t i = 0; i <= total_block - frame_cnt; i++) {
        if(!test_bit(i)) {
            // check the rest of the bits
            uint32_t k = 1;
            for(; k < frame_cnt; k++) {
                if(test_bit(i + k)) {
                    // this block is used, so our contiguous run is broken
                    break;
                }
            }

            if(k == frame_cnt)
                return i;

            // we know i+k is used.
            // so the next possible free block is at i+k+1
            i += k;
        }
    }

    return -1;
}

// update used_block
// must be run after done initializing regions
void pmmngr_update_usage() {
    used_block = 0;
    size_t num_chunks = (total_block + 31) / 32;
    for(unsigned i = 0; i < num_chunks; i++) {
        if(bitmap[i] == 0xffffffff) {
            // be careful not to count padding bits
            int bits_in_this_chunk = 32;
            // if this is the last chunk
            if(i == num_chunks - 1) {
                bits_in_this_chunk = total_block % 32;
                if(bits_in_this_chunk == 0) bits_in_this_chunk = 32;
            }
            
            if(bits_in_this_chunk == 32) {
                 used_block += 32;
                 continue;
            }
        }

        for(int j = 0; j < 32; j++) {
            unsigned bit_index = i * 32 + j;
            if(bit_index >= total_block)
                break;

            int bit = 1 << j;
            if((bitmap[i] & bit)) used_block++;
        }
    }
}
size_t pmmngr_get_size() {
    return total_block * MMNGR_PAGE_SIZE;
}
size_t pmmngr_get_used_size() {
    return used_block * MMNGR_PAGE_SIZE;
}
size_t pmmngr_get_free_size() {
    return (total_block - used_block) * MMNGR_PAGE_SIZE;
}

void pmmngr_init_region(physical_addr_t base, size_t size) {
    int start = base / MMNGR_PAGE_SIZE;
    int block = size / MMNGR_PAGE_SIZE;

    for (; block > 0; block--)
        unset_bit(start++);

    set_bit(0); 
}

void pmmngr_deinit_region(physical_addr_t base, size_t size) {
    int start = base / MMNGR_PAGE_SIZE;
    int block = size / MMNGR_PAGE_SIZE;

    for (;block > 0; block--)
        set_bit(start++);
}

void* pmmngr_alloc_block() {
    if(total_block == used_block) return NULL;
    int frame = find_first_free_block();
    if(frame == -1) return NULL;

    set_bit(frame);

    physical_addr_t base = frame * MMNGR_PAGE_SIZE;
    used_block++;

    return (void*)base;
}
void* pmmngr_alloc_multi_block(size_t cnt) {
    if(used_block + cnt > total_block) return NULL;
    int frame = find_first_free(cnt);
    if(frame == -1) return NULL;

    for(uint32_t i = 0; i < cnt; i++)
        set_bit(frame + i);

    physical_addr_t addr = frame * MMNGR_PAGE_SIZE;
    used_block += cnt;

    return (void*)addr;
}
void pmmngr_free_block(void* base) {
    physical_addr_t addr = (physical_addr_t)base;
    int frame = addr / MMNGR_PAGE_SIZE;

    if(frame == 0) return;

    if(test_bit(frame)) {
        unset_bit(frame);
        used_block--;
    }
}
void pmmngr_free_multi_block(void* base, size_t cnt) {
    physical_addr_t addr = (physical_addr_t)base;
    int frame = addr / MMNGR_PAGE_SIZE;

    if(frame == 0) return;

    for(uint32_t i = 0; i < cnt; i++) {
        if(test_bit(frame + i)) {
            unset_bit(frame + i);
            used_block--;
        }
    }
}

void pmmngr_init(size_t size) {
    total_block = size / MMNGR_PAGE_SIZE;

    // assume that all memory are in use
    used_block = total_block;
    memset(bitmap, 0xFF, sizeof(bitmap));
}
