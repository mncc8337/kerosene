#include "time.h"

#if defined(__is_libk)
#include "timer.h"
#endif

clock_t clock() {
    clock_t c = -1;

#if defined(__is_libk)
    c = timer_get_ticks();
#else
    // TODO: implement libc clock syscall
#endif

    return c;
}
