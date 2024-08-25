#include "video.h"
#include "misc/psf.h"

static bool linear_graphics_mode = false;

// preinit buffers
// the buffer content will be printed after initialise video
static char preinit_buffer[1024];
static unsigned preinit_buffer_len = 0;
static unsigned preinit_cursor = 0;

void video_preinit_set_attr(int fg, int bg) {(void)(fg); (void)(bg);}
void video_preinit_get_size(int* w, int* h) {(void)(w); (void)(h);}
int  video_preinit_rgb(int r, int g, int b) {(void)(r); (void)(g); (void)(b); return 0;}
int  video_preinit_get_cursor() {
    return preinit_cursor;
}
void video_preinit_set_cursor(int offset) {
    preinit_cursor = offset;
}
void video_preinit_cls(int color) {
    (void)(color);
    preinit_buffer_len = 0;
}
void video_preinit_scroll_screen(unsigned ammount) {(void)(ammount);}
void video_preinit_print_char(char chr, int offset, int fg, int bg, bool move) {
    (void)(fg); (void)(bg);
    if(offset < 0) offset = preinit_cursor;
    if((unsigned)offset >= preinit_buffer_len) preinit_buffer_len = offset + 1;

    preinit_buffer[offset++] = chr;
    if(move) preinit_cursor = offset;
}

// actually define the pointers else we would get undefined reference error
void (*video_set_attr)(int fg, int bg) = video_preinit_set_attr;
void (*video_get_size)(int* w, int* h) = video_preinit_get_size;
int (*video_rgb)(int r, int g, int b) = video_preinit_rgb;
int (*video_get_cursor)() = video_preinit_get_cursor;
void (*video_set_cursor)(int offset) = video_preinit_set_cursor;
void (*video_cls)(int color) = video_preinit_cls;
void (*video_scroll_screen)(unsigned ammount) = video_preinit_scroll_screen;
void (*video_print_char)(char chr, int offset, int fg, int bg, bool move) = video_preinit_print_char;

void video_vga_init(uint8_t cols, uint8_t rows) {
    linear_graphics_mode = false;

    video_set_attr      = video_vga_set_attr;
    video_get_size      = video_vga_get_size;
    video_rgb           = video_vga_rgb;
    video_get_cursor    = video_vga_get_cursor;
    video_set_cursor    = video_vga_set_cursor;
    video_cls           = video_vga_cls;
    video_print_char    = video_vga_print_char;
    video_scroll_screen = video_vga_scroll_screen;

    video_vga_set_size(cols, rows);

    if(preinit_buffer_len > 0) {
        for(unsigned i = 0; i < preinit_buffer_len; i++)
            video_vga_print_char(preinit_buffer[i], -1, -1, -1, true);
    }
}
void video_vesa_init(uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp) {
    linear_graphics_mode = true;

    video_set_attr      = video_vesa_set_attr;
    video_get_size      = video_vesa_get_size;
    video_rgb           = video_vesa_rgb;
    video_get_cursor    = video_vesa_get_cursor;
    video_set_cursor    = video_vesa_set_cursor;
    video_cls           = video_vesa_cls;
    video_print_char    = video_vesa_print_char;
    video_scroll_screen = video_vesa_scroll_screen;

    video_vesa_set_size(pitch, bpp, width, height);

    int font_width, font_height, font_bpg;
    psf_get_font_geometry(&font_width, &font_height, &font_bpg);
    video_vesa_set_font_size(font_width, font_height, font_bpg);

    if(preinit_buffer_len > 0) {
        for(unsigned i = 0; i < preinit_buffer_len; i++)
            video_vesa_print_char(preinit_buffer[i], -1, -1, -1, true);
    }
}

bool video_using_framebuffer() {
    return linear_graphics_mode;
}
