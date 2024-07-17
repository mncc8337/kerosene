#pragma once

#include "system.h"

#define PORT_PIT0_DATA 0x40
#define PORT_PIT2_DATA 0x41
#define PORT_PIT3_DATA 0x42
#define PORT_PIT_COMM 0x43

#define PIT_MODE_INT_ON_TERM_CNT          0
#define PIT_MODE_HW_RETRIGGERABLE_ONESHOT 1
#define PIT_MODE_RATE_GEN                 2
#define PIT_MODE_SQRW_GEN                 3
#define PIT_MODE_SW_TRIGGERED_STROBE      4
#define PIT_MODE_HW_TRIGGERED_STROBE      5

void pit_timer_phase(int hz);
