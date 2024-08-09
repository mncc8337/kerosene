#include "timer.h"
#include "system.h"
#include "pit.h"

static volatile unsigned int ticks = 0;
static bool initialised = false;
static void (*tick_listener)(unsigned int) = 0;

static void tick_handler(regs_t* r) {
    (void)(r); // avoid unused arg

    ticks++;
    if(tick_listener) tick_listener(ticks);
}

unsigned int timer_get_ticks() {
    return ticks;
}

void install_tick_listener(void (*tlis)(unsigned int)) {
    tick_listener = tlis;
}
void uninstall_tick_listener() {
    tick_listener = 0;
}

void timer_wait(unsigned int ms) {
    if(!initialised) return;

    unsigned long eticks = ticks + ms * TIMER_FREQUENCY / 1000;
    while(ticks < eticks)
        asm volatile("sti; hlt; cli");
    asm volatile("sti");
}

void timer_init() {
    if(initialised) return;

    pit_timer_frequency(TIMER_FREQUENCY);
    irq_install_handler(0, tick_handler);
    initialised = true;
}
