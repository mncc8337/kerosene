#include "stdio.h"

#if defined(__is_libk)
#include "video.h"
#else
#include "syscall.h"
#endif

int putchar(int ic) {
#if defined(__is_libk)
    video_print_char((char)ic, -1, -1, -1, true);
#else
    int ret;
    SYSCALL_1P(SYSCALL_PUTCHAR, ret, (char)ic);
#endif
    return ic;
}
