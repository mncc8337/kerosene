#include <time.h>
#include <stdbool.h>

#define SECS_PER_DAY 86400

// 1/1/1970 was thursday
#define EPOCH_WEEKDAY 4

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

struct tm gmtime(const time_t* timer) {
    // TODO: return a pointer

    struct tm t;
    time_t clone = *timer;

    t.tm_sec = 0;
    t.tm_min = 0;
    t.tm_hour = 0;
    t.tm_mday = 1;
    t.tm_mon = 1;
    t.tm_year = 1970;

    // i tried doing binary search on this one and i got y2k38'ed lmao
    while(true) {
        int sec = 365 * SECS_PER_DAY;
        if(is_leap_year(t.tm_year)) sec += SECS_PER_DAY;
        if(clone >= sec) {
            clone -= sec;
            t.tm_year++;
        }
        else break;
    }

    t.tm_yday = clone / SECS_PER_DAY;

    bool leap_year = is_leap_year(t.tm_year);
    while(true) {
        int sec = days_in_month(t.tm_mon, leap_year) * SECS_PER_DAY;
        if(clone >= sec) {
            clone -= sec;
            t.tm_mon++;
        }
        else break;
    }

    t.tm_mday += clone / SECS_PER_DAY;
    clone = clone % SECS_PER_DAY;

    t.tm_hour = clone / 3600;
    clone = clone % 3600;

    t.tm_min = clone / 60;
    t.tm_sec = clone % 60;

    int eday = 0; // days since epoch
    eday = (t.tm_year - 1970) * 365;
    eday += (int)((t.tm_year-1)/4)   - (int)(1970/4);
    eday -= (int)((t.tm_year-1)/100) - (int)(1970/100);
    eday += (int)((t.tm_year-1)/400) - (int)(1970/400);
    eday += t.tm_yday;
    t.tm_wday = (EPOCH_WEEKDAY + eday) % 7;

    return t;
}
