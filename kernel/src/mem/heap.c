#include "mem.h"

// heap implementation using first-fit algorithm
// allocating complexity is O(n)
// freeing complexity is O(1)
// very prone to fragmentation

#define MIN_REGION_SIZE 4
#define FREE_RATIO 2/5

heap_t* heap_new(uint32_t start, uint32_t size, size_t max_size, uint8_t flags) {
    // map heap
    physical_addr_t phys = (physical_addr_t)pmmngr_alloc_multi_block(size / MMNGR_PAGE_SIZE);
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

    heap_header_t* header = HEAP_FIRST_HEADER(heap);
    header->magic = HEAP_FREE;
    header->size = heap->end - (uint32_t)header - sizeof(heap_header_t);
    header->prev = NULL;

    return heap;
}

// expand the heap
bool heap_expand(heap_t* heap, size_t page_count, heap_header_t* last_header) {
    if(heap->end + page_count * MMNGR_PAGE_SIZE > heap->max_addr) return true;

    physical_addr_t new_page = (physical_addr_t)pmmngr_alloc_multi_block(page_count);
    if(!new_page) return true;

    int flags = PTE_PRESENT;
    if(!(heap->flags & HEAP_SUPERVISOR)) flags |= PTE_USER;
    if(!(heap->flags & HEAP_READONLY)) flags |= PTE_WRITABLE;
    for(unsigned i = 0; i < page_count; i++)
        vmmngr_map(NULL, new_page + i * MMNGR_PAGE_SIZE, heap->end + i * MMNGR_PAGE_SIZE, flags);

    // assume that last_header is valid

    if(last_header->magic == HEAP_FREE) {
        last_header->size += page_count * MMNGR_PAGE_SIZE;
    }
    else {
        // add new region
        heap_header_t* header = (heap_header_t*)heap->end;
        header->magic = HEAP_FREE;
        header->size = page_count * MMNGR_PAGE_SIZE - sizeof(heap_header_t);
        header->prev = last_header;
    }

    heap->end += page_count * MMNGR_PAGE_SIZE;

    return false;
}

// contract heap, please ensure that the last_header size is larger that page_count * MMNGR_PAGE_SIZE + MIN_REGION_SIZE
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
        vmmngr_unmap(NULL, heap->end);
        page_count--;
    }
}

void* heap_alloc(heap_t* heap, size_t size, bool page_align) {
    heap_header_t* header = HEAP_FIRST_HEADER(heap);

    size_t reg_size = size + sizeof(heap_header_t);
    heap_header_t* final_header = 0;
    while((uint32_t)header < heap->end) {
        if(header->magic != HEAP_FREE) goto next_header;

        if(page_align) {
            uint32_t addr = (uint32_t)header + sizeof(heap_header_t);
            size_t new_size = reg_size;
            if(addr % MMNGR_PAGE_SIZE > 0)
                new_size += MMNGR_PAGE_SIZE - addr % MMNGR_PAGE_SIZE;

            if(header->size >= new_size) break;
        }
        else if(header->size >= reg_size) break;

        next_header:
        final_header = header;
        header = HEAP_NEXT_HEADER(header);
    }
    if((uint32_t)header >= heap->end) {
        // new memory region should start at the end
        header = (heap_header_t*)heap->end;

        // ensure that we have enough memory after expanding
        unsigned needed_size = 0;
        if(final_header->magic == HEAP_USED) needed_size = size + sizeof(heap_header_t);
        else needed_size = size - final_header->size;

        if(needed_size % MMNGR_PAGE_SIZE > 0)
            needed_size += MMNGR_PAGE_SIZE - needed_size % MMNGR_PAGE_SIZE;

        bool err = heap_expand(heap, needed_size / MMNGR_PAGE_SIZE, final_header);
        if(err) return 0;

        header = final_header;
    }

    size_t temp_size = size;
    uint32_t page_aligned_addr = (uint32_t)header + sizeof(heap_header_t);
    if(page_align && page_aligned_addr % MMNGR_PAGE_SIZE > 0) {
        size_t offset = MMNGR_PAGE_SIZE - page_aligned_addr % MMNGR_PAGE_SIZE;
        temp_size += offset;
        page_aligned_addr += offset;
    }

    size_t spare_bytes = header->size - size - sizeof(heap_header_t);
    // only split into 2 regions if the remain size is sufficient
    if(spare_bytes >= MIN_REGION_SIZE) {
        heap_header_t* newh = (heap_header_t*)((void*)header + sizeof(heap_header_t) + temp_size);
        newh->magic = HEAP_FREE;
        newh->size = spare_bytes;
        newh->prev = header;

        // update next header
        heap_header_t* n = HEAP_NEXT_HEADER(newh);
        if((uint32_t)n < heap->end) n->prev = newh;

        header->size = size;
    }
    header->magic = HEAP_USED;

    if(page_align) {
        heap_header_t* newh = (heap_header_t*)(page_aligned_addr - sizeof(heap_header_t));
        newh->magic = HEAP_USED;
        newh->size = size;
        header->magic = HEAP_FREE;

        // update next header
        heap_header_t* n = HEAP_NEXT_HEADER(newh);
        if((uint32_t)n < heap->end) n->prev = newh;

        // only split if headers are not overlap and size is larger than minimum
        uint32_t diff = (uint32_t)newh - (uint32_t)header;
        if(diff >= sizeof(heap_header_t) + MIN_REGION_SIZE) {
            header->size = diff - sizeof(heap_header_t);
            newh->prev = header;
        }
        else {
            // merge the excess bytes to previous region, used or not used
            heap_header_t* prevh = header->prev;
            prevh->size += diff;
            newh->prev = prevh;
        }

        // swap to return
        header = newh;
    }

    return (void*)header + sizeof(heap_header_t);
}

void heap_free(heap_t* heap, void* addr) {
    // the header is always behind the addr so this should work
    heap_header_t* header = (heap_header_t*)(addr - sizeof(heap_header_t));
    // if the magic does not match or it is not in use then skip
    if(header->magic != HEAP_USED) return;

    heap_header_t* prevh = header->prev;
    heap_header_t* nexth = HEAP_NEXT_HEADER(header);

    header->magic = HEAP_FREE;

    bool next_merged = false;
    // merge with next region
    if((uint32_t)nexth < heap->end && nexth->magic == HEAP_FREE) {
        header->size += nexth->size + sizeof(heap_header_t);

        // update next header
        heap_header_t* n = HEAP_NEXT_HEADER(header);
        if((uint32_t)n < heap->end) n->prev = header;

        next_merged = true;
    }

    bool prev_merged = false;
    // merge with previous region
    if(prevh && prevh->magic == HEAP_FREE) {
        prevh->size += header->size + sizeof(heap_header_t);

        // update next header
        heap_header_t* n = HEAP_NEXT_HEADER(prevh);
        if((uint32_t)n < heap->end) n->prev = prevh;

        prev_merged = true;
    }

    // contracting heap if the last region is free
    if(next_merged) {
        heap_header_t* final_reg;
        if(prev_merged) final_reg = prevh;
        else final_reg = header;

        // contract heap if freesize is larger than FREE_RATIO
        // also ensure that after contracting, the final region will have at least the size of MIN_REGION_SIZE
        unsigned ideal_size = (heap->end - (uint32_t)heap) * FREE_RATIO;
        if((unsigned)final_reg + sizeof(heap_header_t) + MIN_REGION_SIZE <= (uint32_t)heap + ideal_size)
            heap_contract(heap, ideal_size / MMNGR_PAGE_SIZE, final_reg);
    }

}
