#include "data_structure/ordered_array.h"

// TODO: better ordered array implementation using binary tree

static bool _less_than(void* a, void* b) {
    return (a < b ? true : false);
}

ordered_array_t ordered_array_t_new(void* ptr, int max_size, bool (*cmp)(void* a, void* b)) {
    ordered_array_t oa;
    oa.array = ptr;
    oa.size = 0;
    oa.max_size = max_size;
    if(cmp == 0) cmp = _less_than;
    oa.cmp = cmp;

    return oa;
}

void ordered_array_t_insert(ordered_array_t* oa, void* item) {
    if(oa->size == oa->max_size) return;

    for(int i = 0; i < oa->size; i++) {
        if(oa->cmp(oa->array + i, item)) continue;
        i--;

        // shift to the right
        for(int k = oa->size - 1; k > i+1; k--)
            oa->array[k] = oa->array[k-1];

        oa->array[i+1] = item;
        oa->size++;
        return;
    }

    // if it reached this point then just insert it at the end
    oa->array[oa->size++] = item;
}

void ordered_array_t_remove(ordered_array_t* oa, int idx) {
    if(oa->size == 0 || idx >= oa->size) return;

    // shift to the left
    for(int i = idx; i < oa->size; i++)
        oa->array[i] = oa->array[i+1];
    oa->size--;
}
