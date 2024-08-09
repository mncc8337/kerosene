#pragma once

#include "stdbool.h"

#define TIMER_FREQUENCY 1000

unsigned int timer_get_ticks();
int timer_get_start_time();
int timer_get_current_time();

void install_tick_listener(void (*tlis)(unsigned int));
void uninstall_tick_listener();

void timer_wait(unsigned int waittick);

bool timer_is_initialised();
void timer_init();
