#pragma once

#include "stdbool.h"

typedef struct {
    void** array;
    int size;
    int max_size;
    bool (*cmp)(void* a, void* b);
} ordered_array_t;

ordered_array_t ordered_array_t_new(void* ptr, int max_size, bool (*cmp)(void* a, void* b));
void ordered_array_t_insert(ordered_array_t* oa, void* item);
void ordered_array_t_remove(ordered_array_t* oa, int idx);
