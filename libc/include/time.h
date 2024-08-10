#pragma once

#include "stdint.h"

#if defined(__is_libk)
#define CLOCKS_PER_SEC TIMER_FREQUENCY
#else
// this is just a placeholder
#define CLOCKS_PER_SEC 1000
#endif


// epochalypse is still 14 years away :)
typedef int32_t time_t;

typedef int32_t clock_t;

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
double difftime(time_t time1, time_t time0);
clock_t clock();

struct tm gmtime(const time_t* timer);
