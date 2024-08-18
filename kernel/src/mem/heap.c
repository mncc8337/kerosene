#include "mem.h"
#include "data_structure/ordered_array.h"

#define HEAP_HEADER_MAGIC 0xdeadbee0
#define HEAP_FOOTER_MAGIC 0xbad1ceae

typedef struct {
    uint32_t magic;
    uint32_t is_hole;
    uint32_t size;
} header_t;

typedef struct {
    uint32_t magic;
    header_t* header;
} footer_t;

typedef struct {
    ordered_array_t index;
    uint32_t start_address;
    uint32_t end_address;
    uint32_t max_address;
    uint8_t supervisor;
    uint8_t readonly;
} heap_t;

void* kmalloc(int byte) {}

void kfree(void* ptr) {}
