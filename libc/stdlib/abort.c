#include "stdlib.h"
#include "stdio.h"

__attribute__((__noreturn__))
void abort(void) {
#if defined(__is_libk)
    // TODO: add proper kernel panic
    printf("kernel: panic: abort()\n");
    asm volatile("hlt");
#else
    // TODO: abnormally terminate the process as if by SIGABRT
    printf("abort()\n");
#endif
    while(1) {} __builtin_unreachable();
}
