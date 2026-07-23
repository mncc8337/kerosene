#include <stdlib.h>
#include <stdio.h>

__attribute__((__noreturn__))
void abort(void) {
    // TODO: abnormally terminate the process as if by SIGABRT
    printf("abort()\n");
    while(1) asm volatile ("pause");
}
