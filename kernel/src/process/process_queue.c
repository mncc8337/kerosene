#include <process.h>

bool process_sort_by_sleep_ticks(process_t* a, process_t* b) {
    return a->sleep_ticks < b->sleep_ticks;
}

void process_queue_push(process_queue_t* procqueue, process_t* proc) {
    proc->queue_next = NULL;

    if(procqueue->top == NULL) procqueue->top = proc;
    else procqueue->bottom->queue_next = proc;

    procqueue->bottom = proc;

    procqueue->size++;
}

void process_queue_sorted_push(process_queue_t* procqueue, process_t* proc, bool (*cmp)(process_t*, process_t*)) {
    if(!procqueue->top) {
        proc->queue_next = NULL;
        procqueue->top = proc;
        procqueue->bottom = proc;
        goto done;
    }

    if(!cmp(procqueue->top, proc)) {
        proc->queue_next = procqueue->top;
        procqueue->top = proc;
        goto done;
    }

    process_t* prev = procqueue->top;
    while(prev->queue_next && cmp(prev->queue_next, proc)) prev = prev->queue_next;
    if(!prev->queue_next) procqueue->bottom = proc;

    proc->queue_next = prev->queue_next;
    prev->queue_next = proc;

    done:
    procqueue->size++;
}

process_t* process_queue_pop(process_queue_t* procqueue) {
    process_t* ret = procqueue->top;
    if(!ret) return ret;

    if(ret == procqueue->bottom) procqueue->top = NULL;
    else procqueue->top = procqueue->top->queue_next;

    procqueue->size--;
    return ret;
}
