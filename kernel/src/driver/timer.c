#include "timer.h"
#include "system.h"
#include "pit.h"
#include "rtc.h"
#include "process.h"

static volatile unsigned ticks = 0;
static time_t start_timestamp;
static volatile time_t seconds_since_start = 0;

static void tick_handler(regs_t* r) {
    ticks++;

    if(ticks > 0 && ticks % TIMER_FREQUENCY == 0) {
        seconds_since_start++;
        ticks = 0;
    }

    scheduler_switch(r);
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

void timer_wait(unsigned ms) {
    unsigned eticks = ticks + ms * TIMER_FREQUENCY / 1000;
    unsigned eseconds = eticks / TIMER_FREQUENCY;
    eticks -= eseconds * TIMER_FREQUENCY;
    eseconds += seconds_since_start;

    while(eseconds > (unsigned)seconds_since_start || eticks > ticks);
}

void timer_init() {
    struct tm t = rtc_get_current_time();
    start_timestamp = mktime(&t);

    pit_timer_frequency(TIMER_FREQUENCY);
    irq_install_handler(0, tick_handler);
}
