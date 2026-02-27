#include <timer.h>
#include <system.h>
#include <pit.h>
#include <rtc.h>
#include <process.h>

static unsigned ticks = 0;
static time_t start_timestamp;
static time_t seconds_since_start = 0;

static uint32_t tick_handler(regs_t* r) {
    ticks++;

    if(ticks == TIMER_FREQUENCY) {
        seconds_since_start++;
        ticks = 0;
    }

    // trigger the scheduler for every tick
    return scheduler_switch(r);
}

time_t timer_get_start_time() {
    return start_timestamp;
}

time_t timer_get_current_time() {
    return start_timestamp + seconds_since_start;
}

time_t timer_get_seconds_since_start() {
    return seconds_since_start;
}

unsigned timer_get_current_ticks() {
    return ticks;
}

void timer_init() {
    struct tm t = rtc_get_current_time();
    start_timestamp = mktime(&t);

    pit_timer_frequency(TIMER_FREQUENCY);
    irq_install_handler(0, tick_handler);
}
