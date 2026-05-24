#pragma once

#include <stdint.h>

uint64_t timer_get_start_time();
uint64_t timer_get_current_time();
void timer_get_current_time_syscall(uint64_t* time);
uint64_t timer_get_seconds_since_start();
uint64_t timer_get_current_ticks();

void timer_init();
