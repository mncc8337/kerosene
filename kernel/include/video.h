#pragma once

#include "stdint.h"
#include "stdbool.h"

#define PORT_SCREEN_CTRL 0x3d4
#define PORT_SCREEN_DATA 0x3d5

#define TTY_MAX_ROWS 25
#define TTY_MAX_COLS 80

#define TTY_BLACK         0x0
#define TTY_BLUE          0x1
#define TTY_GREEN         0x2
#define TTY_CYAN          0x3
#define TTY_RED           0x4
#define TTY_MAGENTA       0x5
#define TTY_BROWN         0x6
#define TTY_LIGHT_GREY    0x7
#define TTY_DARK_GREY     0x8
#define TTY_LIGHT_BLUE    0x9
#define TTY_LIGHT_GREEN   0xa
#define TTY_LIGHT_CYAN    0xb
#define TTY_LIGHT_RED     0xc
#define TTY_LIGHT_MAGENTA 0xd
#define TTY_LIGHT_BROWN   0xe
#define TTY_WHITE         0xf

#define TTY_ATTR(bg, fg) (bg) * 16 + (fg)
#define TTY_OFFSET(r, c) (r) * TTY_MAX_COLS + (c)

void video_set_attr(unsigned char attr);
void video_enable_cursor(unsigned char cursor_scanline_start, unsigned char cursor_scanline_end);
void video_disable_cursor();
int video_get_cursor();
void video_set_cursor(int offset);
void video_cls(char attr);
void video_print_char(char chr, int offset, char attr, bool move);
void video_print_string(char* string, int offset, char attr, bool move);
void video_scroll_screen(unsigned int ammount);
void video_set_vidmem_ptr(uint32_t ptr);
