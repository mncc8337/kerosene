#pragma once

#include "time.h"

time_t timer_get_start_time();
time_t timer_get_current_time();
time_t timer_get_seconds_since_start();
unsigned timer_get_current_ticks();

void install_tick_listener(void (*tlis)(unsigned));
void uninstall_tick_listener();

void timer_wait(unsigned ms);

void timer_init();
