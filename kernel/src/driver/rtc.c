#include "rtc.h"
#include "system.h"

static bool is_update_in_progress() {
    port_outb(PORT_RTC_SELECT_REG, RTC_REG_A);
    return (port_inb(PORT_RTC_DAT) & 0x80);
}

static uint8_t read_reg(uint8_t reg) {
    port_outb(PORT_RTC_SELECT_REG, reg);
    return port_inb(PORT_RTC_DAT);
}

// NOTE:
// the 2 functions below are not working!
// idk why the IRQ 8 only fired once
// TODO: fix RTC timer only fired once
/*
void rtc_timer_frequency(int freq) {
    int pow = 0;
    // freq must be power of 2
    while(freq != 1) {
        freq >>= 1;
        pow++;
    }
    if(pow > 12) return; // rate must be larger than 2
    uint8_t rate = 15 - pow;

    port_outb(PORT_RTC_SELECT_REG, RTC_REG_A | RTC_DISABLE_NMI);
    uint8_t prev = port_inb(PORT_RTC_DAT);
    port_outb(PORT_RTC_SELECT_REG, RTC_REG_A | RTC_DISABLE_NMI);
    port_outb(PORT_RTC_DAT, (prev & 0xf0) | rate);
}

void rtc_timer_start() {
    // NOTE:
    // since the RTC use IRQ 8 (slave)
    // so while other IRQ from 0 to 7 (especially IRQ 0 (PIT) since it is requested a lot)
    // are being handled, others IRQ are discard
    // so this is very unreliable to be used as a timer

    port_outb(PORT_RTC_SELECT_REG, RTC_REG_B | RTC_DISABLE_NMI);
    uint8_t prev = port_inb(PORT_RTC_DAT);
    port_outb(PORT_RTC_SELECT_REG, RTC_REG_B | RTC_DISABLE_NMI);
    port_outb(PORT_RTC_DAT, prev | 0x40);
}
*/

struct tm rtc_get_current_time() {
    struct tm curr_time;

    uint8_t last_second;
    uint8_t last_minute;
    uint8_t last_hour;
    uint8_t last_day;
    uint8_t last_month;
    uint8_t last_year;

    while(is_update_in_progress());
    curr_time.tm_sec  = read_reg(RTC_REG_SECOND);
    curr_time.tm_min  = read_reg(RTC_REG_MINUTE);
    curr_time.tm_hour = read_reg(RTC_REG_HOUR);
    curr_time.tm_mday = read_reg(RTC_REG_DAY);
    curr_time.tm_mon  = read_reg(RTC_REG_MONTH);
    curr_time.tm_year = read_reg(RTC_REG_YEAR);

    do {
        last_second = curr_time.tm_sec;
        last_minute = curr_time.tm_min;
        last_hour   = curr_time.tm_hour;
        last_day    = curr_time.tm_mday;
        last_month  = curr_time.tm_mon;
        last_year   = curr_time.tm_year;

        while(is_update_in_progress());
        curr_time.tm_sec  = read_reg(RTC_REG_SECOND);
        curr_time.tm_min  = read_reg(RTC_REG_MINUTE);
        curr_time.tm_hour = read_reg(RTC_REG_HOUR);
        curr_time.tm_mday = read_reg(RTC_REG_DAY);
        curr_time.tm_mon  = read_reg(RTC_REG_MONTH);
        curr_time.tm_year = read_reg(RTC_REG_YEAR);

    } while(last_second    != curr_time.tm_sec
            || last_minute != curr_time.tm_min
            || last_hour   != curr_time.tm_hour
            || last_day    != curr_time.tm_mday
            || last_month  != curr_time.tm_mon
            || last_year   != curr_time.tm_year);


    uint8_t reg_b = read_reg(RTC_REG_B);

    if(!(reg_b & 0x4)) {
        // convert BCD to binary
        curr_time.tm_sec = (curr_time.tm_sec & 0xf) + ((curr_time.tm_sec/16) * 10);
        curr_time.tm_min = (curr_time.tm_min & 0xf) + ((curr_time.tm_min/16) * 10);
        curr_time.tm_hour = ((curr_time.tm_hour & 0xf) + (((curr_time.tm_hour & 0x70)/16) * 10))
                            | (curr_time.tm_hour & 0x80);
        curr_time.tm_mday = (curr_time.tm_mday & 0xf) + ((curr_time.tm_mday/16) * 10);
        curr_time.tm_mon = (curr_time.tm_mon & 0xf) + ((curr_time.tm_mon/16) * 10);
        curr_time.tm_year = (curr_time.tm_year & 0xf) + ((curr_time.tm_year/16) * 10);
    }

    // convert 12h clock to 24h clock
    if(!(reg_b & 0x2) && (curr_time.tm_hour & 0x80))
        curr_time.tm_hour = ((curr_time.tm_hour & 0x7f) + 12) % 24;

    // ok until next century comes :)
    curr_time.tm_year += 2000;

    return curr_time;
}
