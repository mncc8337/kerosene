#include "mem.h"

// heap implementation using first-fit algorithm
// allocating complexity is O(n)
// freeing complexity is O(1)
// very prone to fragmentation

#define MIN_REGION_SIZE 4

heap_t* heap_new(uint32_t start, uint32_t size, size_t max_size, uint8_t flags) {
    // map heap
    physical_addr_t phys = (physical_addr_t)pmmngr_alloc_multi_block(size / MMNGR_PAGE_SIZE);
    virtual_addr_t virt = start;
    int f = 0;
    if(!(flags & HEAP_SUPERVISOR)) f |= PTE_USER;
    if(!(flags & HEAP_READONLY)) f |= PTE_WRITABLE;
    for(unsigned i = 0; i < size; i += MMNGR_PAGE_SIZE)
        vmmngr_map_page(phys + i, virt + i, f);

    heap_t* heap = (heap_t*)start;
    heap->start = start + sizeof(heap_t);
    heap->end = start + size;
    heap->max_addr = start + max_size;
    heap->flags = flags;

    heap_header_t* header = (heap_header_t*)heap->start;
    header->magic = HEAP_FREE;
    header->size = heap->end - heap->start - sizeof(heap_header_t);
    header->prev = NULL;

    return heap;
}

void* heap_alloc(heap_t* heap, size_t size, bool page_align) {
    heap_header_t* header = (heap_header_t*)heap->start;

    while((uint32_t)header < heap->end) {
        // TODO: check for both header magic
        if(header->magic != HEAP_FREE) goto next_header;

        if(page_align) {
            uint32_t addr = (uint32_t)header + sizeof(heap_header_t) + header->size - size;
            // size to be added to page-align it
            size_t spare = addr % MMNGR_PAGE_SIZE;
            if(header->size >= size + spare) break;
        }
        else if(header->size >= size) break;

        next_header:
        header = HEAP_NEXT_HEADER(header);
    }
    if((uint32_t)header >= heap->end) {
        // TODO: request more memory
        return 0;
    }

    if(page_align)
        size += ((uint32_t)header + sizeof(heap_header_t) + header->size - size) % MMNGR_PAGE_SIZE;

    size_t spare_bytes = header->size - size - sizeof(heap_header_t);
    // only split into 2 regions if the remain size is sufficient
    if(spare_bytes >= MIN_REGION_SIZE) {
        heap_header_t* new_header =
            (heap_header_t*)((void*)header + sizeof(heap_header_t) + spare_bytes);
        new_header->magic = HEAP_USED;
        new_header->size = size;
        new_header->prev = header;
        // update next header
        heap_header_t* n = HEAP_NEXT_HEADER(new_header);
        if((uint32_t)n < heap->end) n->prev = new_header;

        // set new size
        header->size = spare_bytes;

        // set current header
        header = new_header;
    }
    else header->magic = HEAP_USED;

    if(page_align)
        return (void*)((uint32_t)header + sizeof(heap_header_t) + header->size - size);
    return (void*)header + sizeof(heap_header_t);
}

void heap_free(heap_t* heap, void* addr) {
    // the header is always behind the addr so this should works
    heap_header_t* header = (heap_header_t*)(addr - sizeof(heap_header_t));
    // if the magic does not match or it is not in use then skip
    if(header->magic != HEAP_USED) return;

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
    if(prevh && prevh->magic == HEAP_FREE) {
        prevh->size += header->size + sizeof(heap_header_t);

        // update next header
        heap_header_t* n = HEAP_NEXT_HEADER(prevh);
        if((uint32_t)n < heap->end) n->prev = prevh;
    }
}
