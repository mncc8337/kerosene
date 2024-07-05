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

int get_attr(int bg, int fg);
int get_offset(int row, int col);
void enable_cursor(unsigned char cursor_scanline_start, unsigned char cursor_scanline_end);
void disable_cursor();
int get_cursor();
void set_cursor(int offset);
void cls(char attr);
void print_char(char chr, int offset, char attr, bool move);
void print_string(char* string, int offset, char attr, bool move);
void scroll_screen(unsigned int ammount);
