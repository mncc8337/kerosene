#include "stdio.h"

#if defined(__is_libk)
#include "video.h"
#endif

int putchar(int ic) {
#if defined(__is_libk)
    video_print_char((char)ic, -1, -1, -1, true);
#else
    // TODO: implement stdio and the write system call
#endif
    return ic;
}
