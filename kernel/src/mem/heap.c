#include "mem.h"

// simple heap implementation

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
    if(spare_bytes >= 4) {
        heap_header_t* new_header =
            (heap_header_t*)((void*)header + sizeof(heap_header_t) + spare_bytes);
        new_header->magic = HEAP_USED;
        new_header->size = size;

        header->size = spare_bytes;

        // swap
        heap_header_t* t = header;
        header = new_header;
        new_header = t;
    }
    else header->magic = HEAP_USED;

    if(page_align)
        return (void*)((uint32_t)header + sizeof(heap_header_t) + header->size - size);
    return (void*)header + sizeof(heap_header_t);
}

void heap_free(heap_t* heap, void* addr) {
    ;
}
