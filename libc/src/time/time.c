#include <time.h>
#include <sys/syscall.h>

time_t time(time_t* timer) {
    time_t curr_time = syscall_time();

    if(timer != NULL)
        *timer = curr_time;
    return curr_time;
}
