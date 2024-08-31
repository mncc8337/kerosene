#include "stdlib.h"
#include "stdio.h"

#if defined(__is_libk)
#include "system.h"
#endif

__attribute__((__noreturn__))
void abort(void) {
#if defined(__is_libk)
    puts("kernel: abort()");
    kernel_panic(0);
#else
    // TODO: abnormally terminate the process as if by SIGABRT
    printf("abort()\n");
#endif
    while(1) {} __builtin_unreachable();
}
