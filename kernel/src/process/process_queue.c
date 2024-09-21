#include "process.h"

void process_queue_push(process_queue_t* procqueue, process_t* proc) {
    proc->next = NULL;

    if(procqueue->top == NULL) procqueue->top = proc;
    else procqueue->bottom->next = proc;

    procqueue->bottom = proc;

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
