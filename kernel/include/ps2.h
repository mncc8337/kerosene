#pragma once

#include <stdint.h>

#define PORT_PS2_DATA 0x60
#define PORT_PS2_STAT 0x64
#define PORT_PS2_COMM 0x64

#define PS2_ENABLE_1ST_PORT 0xae
#define PS2_DISABLE_1ST_PORT 0xad
#define PS2_ENABLE_2ND_PORT  0xa8
#define PS2_DISABLE_2ND_PORT 0xa7

#define PS2_TEST_1ST_PORT 0xab
#define PS2_TEST_2ND_PORT 0xa9
#define PS2_TEST_CONTROLLER 0xaa

uint8_t ps2_read_data();
void ps2_write_data(uint8_t dat);
uint8_t ps2_read_stat();

void ps2_wait_for_reading_data();
void ps2_wait_for_writing_data();

void ps2_read_from_2nd_port();
