#include "mem.h"

static heap_t* kheap;

bool kheap_init() {
    kheap = heap_new(
        KHEAP_START,
        KHEAP_INITIAL_SIZE,
        KHEAP_MAX_SIZE,
        HEAP_SUPERVISOR
    );

    if(!kheap) return true;
    return false;
}

void* kmalloc(size_t size) {
    return heap_alloc(kheap, size, false);
}

void kfree(void* addr) {
    heap_free(kheap, addr);
}
