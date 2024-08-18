#pragma once

#include "stdint.h"
#include "stdbool.h"

#define PORT_SCREEN_CTRL 0x3d4
#define PORT_SCREEN_DATA 0x3d5

#define VIDEO_TEXTMODE_ADDRESS 0xb8000

#define VIDEO_BLACK         video_rgb(0, 0, 0) // 0x0
#define VIDEO_BLUE          video_rgb(0, 0, 255) // 0x1
#define VIDEO_GREEN         video_rgb(0, 255, 0) // 0x2
#define VIDEO_CYAN          video_rgb(100, 100, 255) // 0x3
#define VIDEO_RED           video_rgb(255, 0, 0) // 0x4
#define VIDEO_MAGENTA       video_rgb(255, 0, 255) // 0x5
#define VIDEO_BROWN         video_rgb(0, 0, 0) // 0x6
#define VIDEO_LIGHT_GREY    video_rgb(150, 150, 150) // 0x7
#define VIDEO_DARK_GREY     video_rgb(60, 60, 60) // 0x8
#define VIDEO_LIGHT_BLUE    video_rgb(100, 100, 255) // 0x9
#define VIDEO_LIGHT_GREEN   video_rgb(100, 255, 100) // 0xa
#define VIDEO_LIGHT_CYAN    video_rgb(150, 150, 255) // 0xb
#define VIDEO_LIGHT_RED     video_rgb(255, 100, 100) // 0xc
#define VIDEO_LIGHT_MAGENTA video_rgb(255, 100, 255) // 0xd
#define VIDEO_LIGHT_BROWN   video_rgb(0, 0, 0) // 0xe
#define VIDEO_WHITE         video_rgb(255, 255, 255) // 0xf

void video_set_attr(int fg, int bg);
void video_set_vidmem_ptr(uint32_t ptr);
void video_size(int* w, int* h);

int  video_textmode_rgb(int r, int g, int b);
void video_textmode_enable_cursor(int cursor_scanline_start, int cursor_scanline_end);
void video_textmode_disable_cursor();
int  video_textmode_get_cursor();
void video_textmode_set_cursor(int offset);
void video_textmode_cls(int bg);
void video_textmode_scroll_screen(unsigned ammount);
void video_textmode_print_char(char chr, int offset, int fg, int bg, bool move);

int  video_framebuffer_rgb(int r, int g, int b);
void video_framebuffer_plot_pixel(int x, int y, int color);
int  video_framebuffer_get_pixel(int x, int y);
int  video_framebuffer_get_cursor();
void video_framebuffer_set_cursor(int offset);
void video_framebuffer_cls(int bg);
void video_framebuffer_scroll_screen(unsigned ammount);
void video_framebuffer_print_char(char chr, int offset, int fg, int bg, bool move);

// function pointers
// that are set to either text mode or linear graphics version of it (specified using the init functions below)
// since they are pointers we need to use the keyword extern
extern int  (*video_rgb)(int r, int g, int b);
extern int  (*video_get_cursor)();
extern void (*video_set_cursor)(int offset);
extern void (*video_cls)(int bg);
extern void (*video_scroll_screen)(unsigned ammount);
extern void (*video_print_char)(char chr, int offset, int fg, int bg, bool move);

void video_textmode_init(uint8_t cols, uint8_t rows);
void video_framebuffer_init(uint32_t pitch, uint32_t width, uint32_t height, uint8_t bpp);
