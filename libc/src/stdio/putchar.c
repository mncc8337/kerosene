#include <system.h>
#include <stdio.h>

#include <stdatomic.h>

#if defined(__is_libk)
#include <video.h>
#else
#include <syscall.h>
#endif

volatile atomic_flag lock;

int putchar(int ic) {
#if defined(__is_libk)
    spinlock_acquire(&lock);
    video_print_char((char)ic, -1, -1, -1, true);
    spinlock_release(&lock);
#else
    int ret;
    SYSCALL_3P(SYSCALL_WRITE, ret, 0, (uint8_t*)&ic, 1);
#endif
    return ic;
}
