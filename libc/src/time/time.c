#include <time.h>
#include <stddef.h>

#if defined(__is_libk)
#include <timer.h>
#else
#include <syscall.h>
#endif

time_t time(time_t* timer) {
    time_t curr_time = -1;

#if defined(__is_libk)
    curr_time = timer_get_current_time();
#else
    SYSCALL_1P(SYSCALL_TIME, curr_time, timer);
#endif

    if(timer != NULL)
        *timer = curr_time;
    return curr_time;
}
