#include <timer.h>
#include <system.h>
#include <pit.h>
#include <rtc.h>
#include <process.h>

static uint64_t ticks = 0;
static uint64_t start_timestamp;
static uint64_t seconds_since_start = 0;

static uint32_t tick_handler(regs_t* r) {
    ticks++;

    if(ticks == TIMER_FREQUENCY) {
        seconds_since_start++;
        ticks = 0;
    }

    // trigger the scheduler for every tick
    return scheduler_switch(r);
}

uint64_t timer_get_start_time() {
    return start_timestamp;
}

uint64_t timer_get_current_time() {
    // because we are now using uint64_t
    // the adding operation is not atomic any more
    // so disabling interrupts is required
    uint32_t eflags;
    asm volatile("pushf; pop %0; cli" : "=r"(eflags));

    uint64_t value = start_timestamp + seconds_since_start;

    asm volatile("push %0; popf" : : "r"(eflags));
    return value;
}

void timer_get_current_time_syscall(uint64_t* time) {
    if(!time) return;

    process_t* proc = scheduler_get_current_process();
    if(proc->is_user && (uint32_t)time >= KERNEL_START) return;

    *time = timer_get_current_time();
}

uint64_t timer_get_seconds_since_start() {
    return seconds_since_start;
}

uint64_t timer_get_current_ticks() {
    return ticks;
}

void timer_init() {
    struct tm t = rtc_get_current_time();
    start_timestamp = mktime(&t);

    pit_timer_frequency(TIMER_FREQUENCY);
    irq_install_handler(0, tick_handler);
}
