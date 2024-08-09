#include "time.h"
#include "stddef.h"

#if defined(__is_libk)
#include "rtc.h"
#include "timer.h"
#endif

// TODO: fix this :(
time_t time(time_t* timer) {
    time_t curr_time = -1;

#if defined(__is_libk)
    // getting infomation fron RTC is really slow so only do it if timer is not initialised
    if(timer_is_initialised()) curr_time = timer_get_current_time();
    else curr_time = rtc_get_current_time();
#else
    // TODO: implement libc time syscall
#endif

    if(timer != NULL)
        *timer = curr_time;
    return curr_time;
}
