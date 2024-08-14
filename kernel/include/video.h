#pragma once

#include "stdint.h"
#include "stdbool.h"

#define PORT_SCREEN_CTRL 0x3d4
#define PORT_SCREEN_DATA 0x3d5

#define VIDEO_TEXTMODE_ADDRESS 0xb8000

#define VIDEO_BLACK         0x0
#define VIDEO_BLUE          0x1
#define VIDEO_GREEN         0x2
#define VIDEO_CYAN          0x3
#define VIDEO_RED           0x4
#define VIDEO_MAGENTA       0x5
#define VIDEO_BROWN         0x6
#define VIDEO_LIGHT_GREY    0x7
#define VIDEO_DARK_GREY     0x8
#define VIDEO_LIGHT_BLUE    0x9
#define VIDEO_LIGHT_GREEN   0xa
#define VIDEO_LIGHT_CYAN    0xb
#define VIDEO_LIGHT_RED     0xc
#define VIDEO_LIGHT_MAGENTA 0xd
#define VIDEO_LIGHT_BROWN   0xe
#define VIDEO_WHITE         0xf

#define VIDEO_ATTR(bg, fg) (bg) * 16 + (fg)

void video_set_attr(uint8_t attr);
void video_set_vidmem_ptr(uint32_t ptr);

void video_textmode_enable_cursor(uint8_t cursor_scanline_start, uint8_t cursor_scanline_end);
void video_textmode_disable_cursor();
int  video_textmode_get_cursor();
void video_textmode_set_cursor(int offset);
void video_textmode_cls(char attr);
void video_textmode_scroll_screen(unsigned ammount);
void video_textmode_print_char(char chr, int offset, char attr, bool move);
void video_textmode_print_string(char* string, int offset, char attr, bool move);

void video_framebuffer_enable_cursor(uint8_t start, uint8_t end);
void video_framebuffer_disable_cursor();
int  video_framebuffer_get_cursor();
void video_framebuffer_set_cursor(int offset);
void video_framebuffer_cls(char attr);
void video_framebuffer_scroll_screen(unsigned ammount);
void video_framebuffer_print_char(char chr, int offset, char attr, bool move);
void video_framebuffer_print_string(char* string, int offset, char attr, bool move);

void video_framebuffer_plot_pixel(int x, int y, int color);

// function pointers
// that are set to either text mode or linear graphics version of it (specified using the init functions below)
// since they are pointers we need to use the keyword extern
extern void (*video_enable_cursor)(uint8_t cursor_scanline_start, uint8_t cursor_scanline_end);
extern void (*video_disable_cursor)();
extern int  (*video_get_cursor)();
extern void (*video_set_cursor)(int offset);
extern void (*video_cls)(char attr);
extern void (*video_scroll_screen)(unsigned ammount);
extern void (*video_print_char)(char chr, int offset, char attr, bool move);
extern void (*video_print_string)(char* string, int offset, char attr, bool move);

void video_textmode_init(uint8_t cols, uint8_t rows);
void video_framebuffer_init(uint32_t pitch, uint32_t width, uint32_t height, uint8_t bpp);
