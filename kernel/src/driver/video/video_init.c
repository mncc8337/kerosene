#include "video.h"
#include "misc/psf.h"

// actually define the pointers else we would get undefined reference error
void (*video_set_attr)(int fg, int bg);
void (*video_get_size)(int* w, int* h);
int  (*video_rgb)(int r, int g, int b);
int  (*video_get_cursor)();
void (*video_set_cursor)(int offset);
void (*video_cls)(int color);
void (*video_scroll_screen)(unsigned ammount);
void (*video_print_char)(char chr, int offset, int fg, int bg, bool move);

static bool linear_graphics_mode = false;

void video_textmode_init(uint8_t cols, uint8_t rows) {
    linear_graphics_mode = false;

    video_set_attr       = video_textmode_set_attr;
    video_get_size       = video_textmode_get_size;
    video_rgb            = video_textmode_rgb;
    video_get_cursor     = video_textmode_get_cursor;
    video_set_cursor     = video_textmode_set_cursor;
    video_cls            = video_textmode_cls;
    video_print_char     = video_textmode_print_char;
    video_scroll_screen  = video_textmode_scroll_screen;

    video_textmode_set_size(cols, rows);
}
void video_framebuffer_init(uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp) {
    linear_graphics_mode = true;

    video_set_attr       = video_framebuffer_set_attr;
    video_get_size       = video_framebuffer_get_size;
    video_rgb            = video_framebuffer_rgb;
    video_get_cursor     = video_framebuffer_get_cursor;
    video_set_cursor     = video_framebuffer_set_cursor;
    video_cls            = video_framebuffer_cls;
    video_print_char     = video_framebuffer_print_char;
    video_scroll_screen  = video_framebuffer_scroll_screen;

    video_framebuffer_set_size(pitch, bpp, width, height);

    int font_width, font_height, font_bpg;
    psf_get_font_geometry(&font_width, &font_height, &font_bpg);
    video_framebuffer_set_font_size(font_width, font_height, font_bpg);
}

bool video_using_framebuffer() {
    return linear_graphics_mode;
}
