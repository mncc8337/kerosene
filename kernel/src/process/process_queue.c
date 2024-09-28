#include "process.h"

bool process_sort_by_sleep_ticks(process_t* a, process_t* b) {
    return a->sleep_ticks < b->sleep_ticks;
}

void process_queue_push(process_queue_t* procqueue, process_t* proc) {
    proc->next = NULL;

    if(procqueue->top == NULL) procqueue->top = proc;
    else procqueue->bottom->next = proc;

    procqueue->bottom = proc;

    procqueue->size++;
}

void process_queue_sorted_push(process_queue_t* procqueue, process_t* proc, bool (*cmp)(process_t*, process_t*)) {
    if(!procqueue->top) {
        proc->next = NULL;
        procqueue->top = proc;
        procqueue->bottom = proc;
        goto done;
    }

    if(!cmp(procqueue->top, proc)) {
        proc->next = procqueue->top;
        procqueue->top = proc;
        goto done;
    }

    process_t* prev = procqueue->top;
    while(prev->next && cmp(prev->next, proc)) prev = prev->next;
    if(!prev->next) procqueue->bottom = proc;

    proc->next = prev->next;
    prev->next = proc;

    done:
    procqueue->size++;
}

process_t* process_queue_pop(process_queue_t* procqueue) {
    process_t* ret = procqueue->top;
    if(!ret) return ret;

    if(ret == procqueue->bottom) procqueue->top = NULL;
    else procqueue->top = procqueue->top->next;

    procqueue->size--;
    return ret;
}
