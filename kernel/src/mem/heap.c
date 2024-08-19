#include "mem.h"

#define HEAP_HEADER_MAGIC 0xdeadbee0
#define HEAP_FOOTER_MAGIC 0xbad1ceae

typedef struct {
    uint32_t magic;
    bool is_hole;
    size_t size;
} header_t;

typedef struct {
    uint32_t magic;
    header_t* header;
} footer_t;

heap_t kheap;

void* heap_malloc(heap_t* heap, size_t byte) {}

void heap_free(heap_t* heap, void* ptr) {}

heap_t* heap_create(virtual_addr_t start, virtual_addr_t end, size_t size, bool supervisor, bool readonly) {}
