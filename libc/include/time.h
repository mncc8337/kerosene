#pragma once

#include <sys/types.h>

#define CLOCKS_PER_SEC TIMER_FREQUENCY

#ifndef NULL
#define NULL ((void *)0)
#endif

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
