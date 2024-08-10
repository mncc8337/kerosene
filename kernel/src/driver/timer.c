#include "timer.h"
#include "system.h"
#include "pit.h"
#include "rtc.h"

static volatile unsigned int ticks = 0;
static void (*tick_listener)(unsigned int) = 0;
static uint32_t start_timestamp;

static void tick_handler(regs_t* r) {
    (void)(r); // avoid unused arg

    ticks++;

    if(tick_listener) tick_listener(ticks);
}

unsigned int timer_get_ticks() {
    return ticks;
}

time_t timer_get_start_time() {
    return start_timestamp;
}

time_t timer_get_current_time() {
    return start_timestamp + (ticks / TIMER_FREQUENCY);
}

void install_tick_listener(void (*tlis)(unsigned int)) {
    tick_listener = tlis;
}
void uninstall_tick_listener() {
    tick_listener = 0;
}

void timer_wait(unsigned int ms) {
    unsigned long eticks = ticks + ms * TIMER_FREQUENCY / 1000;
    while(ticks < eticks)
        asm volatile("sti; hlt; cli");
    asm volatile("sti");
}

void timer_init() {
    struct tm t = rtc_get_current_time();
    start_timestamp = mktime(&t);

    pit_timer_frequency(TIMER_FREQUENCY);
    irq_install_handler(0, tick_handler);
}
