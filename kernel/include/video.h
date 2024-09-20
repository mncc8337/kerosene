#pragma once

#include "stdint.h"
#include "stdbool.h"

#define PORT_SCREEN_CTRL 0x3d4
#define PORT_SCREEN_DATA 0x3d5

#define VIDEO_TEXTMODE_ADDRESS 0xb8000

#define VIDEO_VGA_BLACK         0x0
#define VIDEO_VGA_BLUE          0x1
#define VIDEO_VGA_GREEN         0x2
#define VIDEO_VGA_CYAN          0x3
#define VIDEO_VGA_RED           0x4
#define VIDEO_VGA_MAGENTA       0x5
#define VIDEO_VGA_YELLOW        0x6
#define VIDEO_VGA_LIGHT_GREY    0x7
#define VIDEO_VGA_DARK_GREY     0x8
#define VIDEO_VGA_LIGHT_BLUE    0x9
#define VIDEO_VGA_LIGHT_GREEN   0xa
#define VIDEO_VGA_LIGHT_CYAN    0xb
#define VIDEO_VGA_LIGHT_RED     0xc
#define VIDEO_VGA_LIGHT_MAGENTA 0xd
#define VIDEO_VGA_LIGHT_YELLOW  0xe
#define VIDEO_VGA_WHITE         0xf

#define VIDEO_BLACK         0  , 0  , 0
#define VIDEO_BLUE          0  , 0  , 170
#define VIDEO_GREEN         0  , 170, 0
#define VIDEO_CYAN          0  , 170, 170
#define VIDEO_RED           170, 0  , 0
#define VIDEO_MAGENTA       170, 0  , 170
#define VIDEO_YELLOW        170, 85 , 0
#define VIDEO_LIGHT_GREY    170, 170, 170
#define VIDEO_DARK_GREY     85 , 85 , 85
#define VIDEO_LIGHT_BLUE    85 , 85 , 255
#define VIDEO_LIGHT_GREEN   85 , 255, 85
#define VIDEO_LIGHT_CYAN    85 , 255, 255
#define VIDEO_LIGHT_RED     255, 85 , 85
#define VIDEO_LIGHT_MAGENTA 255, 85 , 255
#define VIDEO_LIGHT_YELLOW  255, 255, 85
#define VIDEO_WHITE         255, 255, 255

// vga.c
void video_vga_set_ptr(int ptr);
void video_vga_set_attr(int fg, int bg);
void video_vga_get_size(int* w, int* h);
void video_vga_set_size(int w, int h);
void video_vga_enable_cursor(int cursor_scanline_start, int cursor_scanline_end);
void video_vga_disable_cursor();
int video_vga_rgb(int r, int g, int b);
int video_vga_get_cursor();
void video_vga_set_cursor(int offset);
void video_vga_cls(int bg);
void video_vga_scroll_screen(unsigned ammount);
void video_vga_print_char(char chr, int offset, int fg, int bg, bool move);

// vesa.c
void video_vesa_set_ptr(int ptr);
void video_vesa_set_attr(int fg, int bg);
void video_vesa_get_size(int* w, int* h);
void video_vesa_set_size(int pitch, int bpp, int w, int h);
void video_vesa_set_font_size(int cw, int ch, int bpg);
void video_vesa_get_font_size(int* w, int* h);
bool video_vesa_set_font(char* font_data);
void video_vesa_get_rowcol(int* c, int* r);
void video_vesa_plot_pixel(unsigned x, unsigned y, int color);
int video_vesa_get_pixel(unsigned x, unsigned y);
void video_vesa_fill_rectangle(int x0, int y0, int x1, int y1, int color);
void video_vesa_draw_line(int x0, int y0, int x1, int y1, int color);
void video_vesa_draw_circle(int x0, int y0, int r, int color);
int video_vesa_rgb(int r, int g, int b);
int video_vesa_get_cursor();
void video_vesa_set_cursor(int offset);
void video_vesa_cls(int bg);
void video_vesa_scroll_screen(unsigned ammount);
void video_vesa_print_char(char chr, int offset, int fg, int bg, bool move);

// function pointers that are set to either text mode or linear graphics version of it
// since they are pointers we need to use the keyword extern
extern void (*video_set_attr)(int fg, int bg);
extern void (*video_get_size)(int* w, int* h);
extern int (*video_rgb)(int r, int g, int b);
extern int (*video_get_cursor)();
extern void (*video_set_cursor)(int offset);
extern void (*video_cls)(int bg);
extern void (*video_scroll_screen)(unsigned ammount);
extern void (*video_print_char)(char chr, int offset, int fg, int bg, bool move);

// video_init.c
void video_preinit_set_attr(int fg, int bg);
void video_preinit_get_size(int* w, int* h);
int  video_preinit_rgb(int r, int g, int b);
int  video_preinit_get_cursor();
void video_preinit_set_cursor(int offset);
void video_preinit_cls(int color);
void video_preinit_scroll_screen(unsigned ammount);
void video_preinit_print_char(char chr, int offset, int fg, int bg, bool move);
void video_vga_init(uint8_t cols, uint8_t rows);
void video_vesa_init(uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp);
bool video_using_framebuffer();
