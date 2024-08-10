#pragma once

#include "time.h"

unsigned int timer_get_ticks();
time_t timer_get_start_time();
time_t timer_get_current_time();

void install_tick_listener(void (*tlis)(unsigned int));
void uninstall_tick_listener();

void timer_wait(unsigned int waittick);

void timer_init();
