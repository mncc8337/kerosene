#pragma once

#define PORT_PIT_CH0 0x40
#define PORT_PIT_CH1 0x41
#define PORT_PIT_CH2 0x42
#define PORT_PIT_COM 0x43

#define PIT_MODE_INT_ON_TERM_CNT          0
#define PIT_MODE_HW_RETRIGGERABLE_ONESHOT 1
#define PIT_MODE_RATE_GEN                 2
#define PIT_MODE_SQRW_GEN                 3
#define PIT_MODE_SW_TRIGGERED_STROBE      4
#define PIT_MODE_HW_TRIGGERED_STROBE      5

void pit_timer_frequency(int hz);
unsigned pit_get_count();
void pit_set_count(unsigned count);

void pit_beep(int freq);
void pit_beep_stop();
