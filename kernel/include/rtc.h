#pragma once

#include "time.h"

#define PORT_RTC_SELECT_REG 0x70
#define PORT_RTC_DAT 0x71

#define RTC_REG_SECOND  0x00
#define RTC_REG_MINUTE  0x02
#define RTC_REG_HOUR    0x04
#define RTC_REG_WEEKDAY 0x06 // do not use, may be incorrect
#define RTC_REG_DAY     0x07
#define RTC_REG_MONTH   0x08
#define RTC_REG_YEAR    0x09
#define RTC_REG_A       0xa
#define RTC_REG_B       0xb

#define RTC_DISABLE_NMI 0x80

void rtc_timer_start();
void rtc_timer_frequency(int freq);
struct tm rtc_get_current_time();
