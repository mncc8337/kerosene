#include <time.h>
#include <stddef.h>

#if defined(__is_libk)
#include <timer.h>
#else
#include <sys/syscall.h>
#endif

time_t time(time_t* timer) {

#if defined(__is_libk)
    time_t curr_time = timer_get_current_time();
#else
    // the line below will call the line above
    // but in a different function lmao
    time_t curr_time = syscall_time(timer);
#endif

    if(timer != NULL)
        *timer = curr_time;
    return curr_time;
}
