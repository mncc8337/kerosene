#include "time.h"

#include "stdbool.h"

static bool is_leap_year(int year) {
    if(year % 400 == 0) return true;
    if(year % 4 == 0) return !(year % 100 == 0);
    return false;
}

static int days_in_month(int month, bool leap_year) {
    if(month <= 7) {
        if(month % 2 == 1) return 31;
        if(month == 2) return 28 + leap_year;
        return 30;
    }
    else {
        if(month % 2 == 0) return 31;
        return 30;
    }
}

// simple mktime that does not account leap second or anything
// that are too complicated for a monkey
time_t mktime(struct tm* tp) {
    time_t t;

    bool leap_year = is_leap_year(tp->tm_year);
    const int seconds_per_day = 86400;

    // calculate total second from 1970 to tp->tm_year
    t = (tp->tm_year - 1970) * 365;
    // accounting leap year
    t += (int)((tp->tm_year-1)/4) - (int)(1970/4);
    t -= (int)((tp->tm_year-1)/100) - (int)(1970/100);
    t += (int)((tp->tm_year-1)/400) - (int)(1970/400);
    t *= seconds_per_day;

    // calculate total second from january to the start of tp->tm_month
    for(int m = 1; m < (tp->tm_mon); m++)
        t += days_in_month(m, leap_year) * seconds_per_day;

    // adding the remaining days (excluding current day)
    t += (tp->tm_mday - 1) * seconds_per_day;

    // the rest is easy
    t += tp->tm_sec;
    t += tp->tm_min * 60;
    t += tp->tm_hour * 3600;

    return t;
}
