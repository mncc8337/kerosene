#pragma once

#define PORT_PIT_CH0 0x40
#define PORT_PIT_CH1 0x41
#define PORT_PIT_CH2 0x42
#define PORT_PIT_COM 0x43

// this port is on the keyboard controller lol
#define PORT_PC_SPEAKER 0x61

void pit_timer_frequency(int hz);
unsigned pit_get_count();
void pit_set_count(unsigned count);

void pit_beep_start();
void pit_beep_stop();
void pit_beep(int freq);
