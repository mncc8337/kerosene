#pragma once

#include "stdint.h"

// epochalypse is still 14 years away :)
typedef uint32_t time_t;

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

time_t time(time_t* timer);
time_t mktime(struct tm* tp);
