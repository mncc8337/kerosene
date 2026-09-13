#include <mem.h>
#include <spinlock.h>
#include <stdint.h>

// heap implementation using first-fit algorithm
// allocating complexity is O(n)
// freeing complexity is O(1)
// very prone to fragmentation

#define MIN_REGION_SIZE 4

heap_t* heap_new(uint32_t start, uint32_t size, size_t max_size, uint8_t flags) {
    // map heap
    paddr_t phys = pmmngr_alloc_multi_block(size / MMNGR_PAGE_SIZE);
    if(!phys) return 0;
    int f = PTE_PRESENT;
    if(!(flags & HEAP_SUPERVISOR)) f |= PTE_USER;
    if(!(flags & HEAP_READONLY)) f |= PTE_WRITABLE;
    for(unsigned i = 0; i < size; i += MMNGR_PAGE_SIZE)
        vmmngr_map(NULL, phys + i, start + i, f);

    heap_t* heap = (heap_t*)start;
    heap->end = start + size;
    heap->max_addr = start + max_size;
    heap->min_size = size;
    heap->flags = flags;
    spinlock_init(&heap->lock);

    heap_header_t* header = HEAP_FIRST_HEADER(heap);
    header->magic = HEAP_FREE;
    header->size = heap->end - (uint32_t)header - sizeof(heap_header_t);
    header->prev = NULL;

    return heap;
}

// expand the heap
bool heap_expand(heap_t* heap, size_t page_count, heap_header_t* last_header) {
    if(heap->end + page_count * MMNGR_PAGE_SIZE > heap->max_addr) return true;

    paddr_t new_page = pmmngr_alloc_multi_block(page_count);
    if(!new_page) return true;

    int flags = PTE_PRESENT;
    if(!(heap->flags & HEAP_SUPERVISOR)) flags |= PTE_USER;
    if(!(heap->flags & HEAP_READONLY)) flags |= PTE_WRITABLE;
    for(unsigned i = 0; i < page_count; i++)
        vmmngr_map(NULL, new_page + i * MMNGR_PAGE_SIZE, heap->end + i * MMNGR_PAGE_SIZE, flags);

    // assume that last_header is valid

    if(last_header->magic == HEAP_FREE) {
        last_header->size += page_count * MMNGR_PAGE_SIZE;
    } else {
        // add new region
        heap_header_t* header = (heap_header_t*)heap->end;
        header->magic = HEAP_FREE;
        header->size = page_count * MMNGR_PAGE_SIZE - sizeof(heap_header_t);
        header->prev = last_header;
    }

    heap->end += page_count * MMNGR_PAGE_SIZE;

    return false;
}

// contract heap, caller must ensure that the last_header size is larger that page_count * MMNGR_PAGE_SIZE + MIN_REGION_SIZE
void heap_contract(heap_t* heap, size_t page_count, heap_header_t* last_header) {
    // recalculate page_count if it is overshoot min_size
    int remain_size = (heap->end - page_count * MMNGR_PAGE_SIZE) - (uint32_t)heap;
    if(remain_size < 0 || (unsigned)remain_size < heap->min_size) {
        page_count = (heap->end - (uint32_t)heap) - heap->min_size;
        page_count /= MMNGR_PAGE_SIZE;
    }

    last_header->size -= page_count * MMNGR_PAGE_SIZE;
    while(page_count > 0) {
        heap->end -= MMNGR_PAGE_SIZE;
        paddr_t phys = vmmngr_to_physical_addr(NULL, heap->end);
        pmmngr_free_block(phys);
        vmmngr_unmap(NULL, heap->end);
        page_count--;
    }
}

void* heap_alloc(heap_t* heap, size_t size, bool page_align) {
    spinlock_acquire(&heap->lock);
    heap_header_t* header = HEAP_FIRST_HEADER(heap);

    size_t reg_size = size + sizeof(heap_header_t);
    heap_header_t* final_header = 0;
    while((uint32_t)header < heap->end) {
        if(header->magic != HEAP_FREE) goto next_header;

        if(page_align) {
            uint32_t addr = (uint32_t)header + sizeof(heap_header_t);
            size_t new_size = reg_size;
            if(addr % MMNGR_PAGE_SIZE > 0) {
                uint32_t offset = MMNGR_PAGE_SIZE - addr % MMNGR_PAGE_SIZE;

                // if there is no prev region to merge with the excessive bytes
                // and the excessive bytes are not large enough to create a new region
                // then just skip
                if(offset < sizeof(heap_header_t) + MIN_REGION_SIZE && header->prev == NULL)
                    goto next_header;

                new_size += offset;
            }

            if(header->size >= new_size) break;
        } else if(header->size >= reg_size) break;

        next_header:
        final_header = header;
        header = HEAP_NEXT_HEADER(header);
    }

    // expand the heap if no suitable region is found
    if((uint32_t)header >= heap->end) {
        // ensure that we have enough memory after expanding
        unsigned needed_size = 0;
        uint32_t potential_addr;
        if(final_header->magic == HEAP_USED) {
            // create a new region at heap->end
            needed_size = size + sizeof(heap_header_t);
            potential_addr = heap->end + sizeof(heap_header_t);
        } else {
            // expand the last header by size - final_header->size
            // final_header->size must be smaller than size since the slot finder code above
            // skip this region
            needed_size = size - final_header->size;
            potential_addr = (uint32_t)final_header + sizeof(heap_header_t);
        }

        // accounting for page aligning padding
        if(page_align && potential_addr % MMNGR_PAGE_SIZE > 0) {
            needed_size += MMNGR_PAGE_SIZE - potential_addr % MMNGR_PAGE_SIZE;
        }

        // round up needed_size
        if(needed_size % MMNGR_PAGE_SIZE > 0)
            needed_size += MMNGR_PAGE_SIZE - needed_size % MMNGR_PAGE_SIZE;

        bool err = heap_expand(heap, needed_size / MMNGR_PAGE_SIZE, final_header);
        if(err) {
            spinlock_release(&heap->lock);
            return 0;
        }

        header = final_header;
    }

    size_t temp_size = size;
    uint32_t page_aligned_addr = (uint32_t)header + sizeof(heap_header_t);
    if(page_align && page_aligned_addr % MMNGR_PAGE_SIZE > 0) {
        size_t offset = MMNGR_PAGE_SIZE - page_aligned_addr % MMNGR_PAGE_SIZE;
        temp_size += offset;
        page_aligned_addr += offset;
    }

    size_t spare_bytes = header->size - temp_size - sizeof(heap_header_t);
    // only split into 2 regions if the remain size is sufficient
    if(spare_bytes >= MIN_REGION_SIZE) {
        heap_header_t* newh = (heap_header_t*)((void*)header + sizeof(heap_header_t) + temp_size);
        newh->magic = HEAP_FREE;
        newh->size = spare_bytes;
        newh->prev = header;

        // update next header
        heap_header_t* n = HEAP_NEXT_HEADER(newh);
        if((uint32_t)n < heap->end) n->prev = newh;

        header->size = temp_size;
    }
    header->magic = HEAP_USED;

    if(page_align) {
        heap_header_t* newh = (heap_header_t*)(page_aligned_addr - sizeof(heap_header_t));
        uint32_t diff = (uint32_t)newh - (uint32_t)header;

        if(diff > 0) {
            // only set if newh and header is different
            newh->magic = HEAP_USED;
            newh->size = size;
            header->magic = HEAP_FREE;

            // update next header
            heap_header_t* n = HEAP_NEXT_HEADER(newh);
            if((uint32_t)n < heap->end) n->prev = newh;

            // only split if headers are not overlap and size is larger than minimum
            if(diff >= sizeof(heap_header_t) + MIN_REGION_SIZE) {
                header->size = diff - sizeof(heap_header_t);
                newh->prev = header;
            } else {
                // merge the excess bytes to previous region, used or not used
                heap_header_t* prevh = header->prev;
                prevh->size += diff;
                newh->prev = prevh;
            }

            // swap to return
            header = newh;
        }
    }

    spinlock_release(&heap->lock);
    return (void*)header + sizeof(heap_header_t);
}

void heap_free(heap_t* heap, void* addr) {
    // the header is always behind the addr so this should work
    heap_header_t* header = (heap_header_t*)(addr - sizeof(heap_header_t));
    // if the magic does not match or it is not in use then skip
    if(header->magic != HEAP_USED) return;

    spinlock_acquire(&heap->lock);

    heap_header_t* prevh = header->prev;
    heap_header_t* nexth = HEAP_NEXT_HEADER(header);

    header->magic = HEAP_FREE;

    // merge with next region
    if((uint32_t)nexth < heap->end && nexth->magic == HEAP_FREE) {
        header->size += nexth->size + sizeof(heap_header_t);

        // update next header
        heap_header_t* n = HEAP_NEXT_HEADER(header);
        if((uint32_t)n < heap->end) n->prev = header;
    }

    // merge with previous region
    bool merge_prev = prevh && prevh->magic == HEAP_FREE;
    if(merge_prev) {
        prevh->size += header->size + sizeof(heap_header_t);

        // update next header
        heap_header_t* n = HEAP_NEXT_HEADER(prevh);
        if((uint32_t)n < heap->end) n->prev = prevh;
    }

    // since the next region can only merged into the current region
    // and the current region can only merged into the prev region
    // then the result region can only be the prev region (if merged) or the current region
    heap_header_t* result_region = (merge_prev) ? prevh : header;

    if((uint32_t)HEAP_NEXT_HEADER(result_region) >= heap->end) {
        // we have merged the final region of the heap into the current region
        // that mean we can contract if possible
        size_t total_size = heap->end - (uint32_t)heap;
        size_t ideal_size = (total_size * 2 / 5) / MMNGR_PAGE_SIZE;

        // contract heap if freesize is larger than FREE_RATIO
        // also ensure that after contracting, the final region will have at least the size of MIN_REGION_SIZE
        size_t contracted_free_size = result_region->size - (ideal_size * MMNGR_PAGE_SIZE); 
        if(ideal_size > 0 && contracted_free_size >= MIN_REGION_SIZE) {
            heap_contract(heap, ideal_size, result_region);
        }
    }

    spinlock_release(&heap->lock);
}
