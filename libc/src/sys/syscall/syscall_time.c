#include <sys/syscall.h>

time_t syscall_time(time_t* timer) {
    time_t curr_time = -1;
    SYSCALL_1P(SYSCALL_TIME, curr_time, timer);

    if(timer != NULL)
        *timer = curr_time;
    return curr_time;
}

