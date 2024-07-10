#include "stdio.h"

#if defined(__is_libk)
#include "tty.h"
#endif

int putchar(int ic) {
#if defined(__is_libk)
    tty_print_char((char)ic, -1, 0, true);
#else
    // TODO: implement stdio and the write system call
#endif
    return ic;
}
