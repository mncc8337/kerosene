#include <system.h>
#include <stdio.h>

#if defined(__is_libk)
#include <video.h>
#else
#include <unistd.h>
#include <syscall.h>
#endif

int putchar(int ic) {
#if defined(__is_libk)
    video_printc((char)ic, -1, -1, -1, true);
#else
    int ret;
    SYSCALL_3P(SYSCALL_WRITE, ret, STDOUT_FILENO, (uint8_t*)&ic, 1);
#endif
    return ic;
}
