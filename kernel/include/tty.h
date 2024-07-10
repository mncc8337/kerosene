#pragma once

#include "system.h"

#define PORT_SCREEN_CTR 0x3d4
#define PORT_SCREEN_DAT 0x3d5

#define MAX_ROWS 25
#define MAX_COLS 80

#define BLACK         0x0
#define BLUE          0x1
#define GREEN         0x2
#define CYAN          0x3
#define RED           0x4
#define MAGENTA       0x5
#define BROWN         0x6
#define LIGHT_GREY    0x7
#define DARK_GREY     0x8
#define LIGHT_BLUE    0x9
#define LIGHT_GREEN   0xa
#define LIGHT_CYAN    0xb
#define LIGHT_RED     0xc
#define LIGHT_MAGENTA 0xd
#define LIGHT_BROWN   0xe
#define WHITE         0xf

int tty_get_attr(int bg, int fg);
int tty_get_offset(int row, int col);
void tty_enable_cursor(unsigned char cursor_scanline_start, unsigned char cursor_scanline_end);
void tty_disable_cursor();
int tty_get_cursor();
void tty_set_cursor(int offset);
void tty_cls(char attr);
void tty_print_char(char chr, int offset, char attr, bool move);
void tty_print_string(char* string, int offset, char attr, bool move);
void tty_scroll_screen(unsigned int ammount);
void tty_set_vidmem_ptr(uint32_t ptr);
