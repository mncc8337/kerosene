#include "mem.h"

// heap implementation using first-fit algorithm
// allocating complexity is O(n)
// freeing complexity is O(1)
// very prone to fragmentation

#define MIN_REGION_SIZE 4

heap_t* heap_new(uint32_t start, uint32_t size, size_t max_size, uint8_t flags) {
    // map heap
    physical_addr_t phys = (physical_addr_t)pmmngr_alloc_multi_block(size / MMNGR_PAGE_SIZE);
    if(!phys) return 0;
    int f = 0;
    if(!(flags & HEAP_SUPERVISOR)) f |= PTE_USER;
    if(!(flags & HEAP_READONLY)) f |= PTE_WRITABLE;
    for(unsigned i = 0; i < size; i += MMNGR_PAGE_SIZE)
        vmmngr_map_page(phys + i, start + i, f);

    heap_t* heap = (heap_t*)start;
    heap->start = start + sizeof(heap_t);
    heap->end = start + size;
    heap->max_addr = start + max_size;
    heap->min_size = size;
    heap->flags = flags;

    heap_header_t* header = (heap_header_t*)heap->start;
    header->magic = HEAP_FREE;
    header->size = heap->end - heap->start - sizeof(heap_header_t);
    header->prev = NULL;

    return heap;
}

bool heap_expand(heap_t* heap, size_t page_count, heap_header_t* last_header) {
    if(heap->end + page_count * MMNGR_PAGE_SIZE > heap->max_addr) return false;

    physical_addr_t new_page = (physical_addr_t)pmmngr_alloc_multi_block(page_count);
    if(!new_page) return false;

    int flags = 0;
    if(!(heap->flags & HEAP_SUPERVISOR)) flags |= PTE_USER;
    if(!(heap->flags & HEAP_READONLY)) flags |= PTE_WRITABLE;
    for(unsigned i = 0; i < page_count; i++)
        vmmngr_map_page(new_page + i * MMNGR_PAGE_SIZE, heap->end + i * MMNGR_PAGE_SIZE, flags);

    // assume that the last_header is valid

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

    return true;
}

// TODO: implement heap_contract
bool heap_contract(heap_t* heap, size_t page_count, heap_header_t* last_header) {
    // the algorithm is simple
    // we first translate each virtual addr to phys
    // and then free them using the pmmngr
    // and finally change the last_header
    // remember to check heap min_size

    // TODO: implement virtual address to physical address translation

    return false;
}

void* heap_alloc(heap_t* heap, size_t size, bool page_align) {
    heap_header_t* header = (heap_header_t*)heap->start;

    size_t reg_size = size + sizeof(heap_header_t);
    heap_header_t* saved_header = 0;
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
        saved_header = header;
        header = HEAP_NEXT_HEADER(header);
    }
    if((uint32_t)header >= heap->end) {
        // new memory region should start at the end
        header = (heap_header_t*)heap->end;

        bool err = heap_expand(heap, 1, saved_header);
        if(err) return 0;
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

    // contract if needed
    if(next_merged) {
        heap_header_t* final_reg;
        if(prev_merged) final_reg = prevh;
        else final_reg = header;

        if((uint32_t)HEAP_NEXT_HEADER(final_reg) > heap->end && final_reg->size >= MMNGR_PAGE_SIZE + MIN_REGION_SIZE)
            heap_contract(heap, 1, final_reg);
    }

}
