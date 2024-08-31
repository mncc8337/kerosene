#include "time.h"

#include "stdint.h"

#define TYPE_SIGNED(type) ((type) -1 < 0)

double difftime(time_t time1, time_t time0) {
    if(!TYPE_SIGNED(time_t))
        return time1 - time0;

    uintmax_t dt = (uintmax_t)time1 - (uintmax_t)time0;
    return (double)dt;
}
