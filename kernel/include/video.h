#pragma once

#define MAX_ROWS 25
#define MAX_COLS 80

int get_offset(int row, int col);
void enable_cursor(unsigned char cursor_scanline_start, unsigned char cursor_scanline_end);
void disable_cursor();
int get_cursor();
void set_cursor(int offset);
void cls(char attr);
void print_char(char chr, int offset, char attr);
void print_string(char* string, int offset, char attr);
void init_video();
