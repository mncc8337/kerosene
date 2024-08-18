#include "video.h"
#include "misc/psf.h"

#include "string.h"

static uint8_t* framebuffer;

static int current_fg;
static int current_bg;

static int text_rows;
static int text_cols;

static uint32_t fb_pitch;
static uint32_t fb_width;
static uint32_t fb_height;
static uint8_t fb_bpp;

static int cursor_posx = 0;
static int cursor_posy = 0;

static int font_width;
static int font_height;
static int font_bpg;

// the pixels behind the cursor
static int cursor_buffer[512];
static int cursor_buffer_posx = 0;
static int cursor_buffer_posy = 0;

void video_framebuffer_set_ptr(int ptr) {
    framebuffer = (uint8_t*)ptr;
}

void video_framebuffer_set_attr(int fg, int bg) {
    current_fg = fg;
    current_bg = bg;
}

void video_framebuffer_set_size(int pitch, int bpp, int w, int h) {
    fb_pitch = pitch;
    fb_bpp = bpp;
    fb_width = w;
    fb_height = h;
}

void video_framebuffer_get_size(int* w, int* h) {
    *w = fb_width;
    *h = fb_height;
}

void video_framebuffer_set_font_size(int cw, int ch, int bpg) {
    font_width = cw;
    font_height = ch;
    text_cols = fb_width / cw;
    text_rows = fb_height / ch;
    font_bpg = bpg;
}

void video_framebuffer_get_font_size(int* w, int* h) {
    *w = font_width;
    *h = font_height;
}

void video_framebuffer_get_rowcol(int* c, int* r) {
    *c = text_cols;
    *r = text_rows;
}

void video_framebuffer_plot_pixel(int x, int y, int color) {
    if(fb_bpp == 8) {
        uint8_t* pixel = framebuffer + y * fb_pitch + x;
        *pixel = color;
    }
    else if(fb_bpp == 16) {
        uint16_t* pixel = (uint16_t*)(framebuffer + y * fb_pitch) + x;
        *pixel = color;
    }
    else if(fb_bpp == 32) {
        uint32_t* pixel = (uint32_t*)(framebuffer + y * fb_pitch) + x;
        *pixel = color;
    }
    else {
        // who the hell use this is not deserved a pixel
    }
}

int video_framebuffer_get_pixel(int x, int y) {
    if(fb_bpp == 8) {
        uint8_t* pixel = framebuffer + y * fb_pitch + x;
        return *pixel;
    }
    else if(fb_bpp == 16) {
        uint16_t* pixel = (uint16_t*)(framebuffer + y * fb_pitch) + x;
        return *pixel;
    }
    else if(fb_bpp == 32) {
        uint32_t* pixel = (uint32_t*)(framebuffer + y * fb_pitch) + x;
         return *pixel;
    }
    else {
        // who the hell use this is not deserved a pixel
        return 0x0;
    }
}

int video_framebuffer_rgb(int r, int g, int b) {
    // TODO: support other bpp mode
    return (r << 16) | (g << 8) | b;
}

// should be called after changing either cursor_posx or cursor_posy
static void _framebuffer_draw_cursor(bool load_buffer) {
    for(int y = 0; y < font_height; y++) {
        for(int x = 0; x < font_width; x++) {
            if(load_buffer)
                video_framebuffer_plot_pixel(
                    x + cursor_buffer_posx * font_width,
                    y + cursor_buffer_posy * font_height,
                    cursor_buffer[y * font_width + x]
                );

            // load new location to buffer
            cursor_buffer[y * font_width + x] = video_framebuffer_get_pixel(
                x + cursor_posx * font_width,
                y + cursor_posy * font_height
            );

            // now draw the cursor
            video_framebuffer_plot_pixel(
                x + cursor_posx * font_width,
                y + cursor_posy * font_height,
                video_framebuffer_rgb(180, 170, 170)
            );
        }
    }

    // load new location
    cursor_buffer_posx = cursor_posx;
    cursor_buffer_posy = cursor_posy;
}

int video_framebuffer_get_cursor() {
    return cursor_posy * text_cols + cursor_posx;
}
void video_framebuffer_set_cursor(int offset) {
    cursor_posy = offset / text_cols;
    cursor_posx = offset % text_cols;
}

void video_framebuffer_cls(int bg) {
    for(unsigned y = 0; y < fb_height; y++)
        for(unsigned x = 0; x < fb_width; x++)
            video_framebuffer_plot_pixel(x, y, bg);
}

void video_framebuffer_scroll_screen(unsigned ammount) {
    if(cursor_posy == 0) return;

    cursor_posy -= ammount;
    cursor_buffer_posy -= ammount;
    if(cursor_posy < 0) {
        ammount += cursor_posy;
        cursor_posy = 0;
        cursor_buffer_posy = 0;
    }

    memcpy(
        (char*)framebuffer,
        (char*)framebuffer + ammount * font_height * fb_pitch,
        cursor_posy * font_height * fb_pitch
    );
    memset(
        (char*)framebuffer + cursor_posy * font_height * fb_pitch,
        0, ammount * font_height * fb_pitch
    );
}

void video_framebuffer_print_char(char chr, int offset, int fg, int bg, bool move) {
    if(chr == 0) return;

    int _cursor_posx = cursor_posx;
    int _cursor_posy = cursor_posy;
    if(fg < 0) fg = current_fg;
    if(bg < 0) bg = current_bg;

    if(offset >= 0) {
        _cursor_posy = offset / text_cols;
        _cursor_posx = offset % text_cols;
    }

    bool load_cursor_buffer = false;
    if(chr == '\n') {
        load_cursor_buffer = true;
        _cursor_posx = 0;
        _cursor_posy++;
    }
    else if(chr == '\b') {
        load_cursor_buffer = true;
        _cursor_posx -= 1;
        if(_cursor_posx < 0) {
            _cursor_posx = text_cols - 1;
            _cursor_posy -= 1;
            if(_cursor_posy < 0) _cursor_posy = 0;
        }
        // manually delete the char
        for(int y = 0; y < font_height; y++)
            for(int x = 0; x < font_width; x++)
                video_framebuffer_plot_pixel(
                    x + _cursor_posx * font_width,
                    y + _cursor_posy * font_height,
                    0x0
                );
    }
    else if(chr == '\t') {
        video_framebuffer_print_char(
            ' ', _cursor_posy * text_cols + _cursor_posx,
            0x0, 0x0, true
        );
        _cursor_posx++;
    }
    else {
        char* glyph = psf_get_glyph(chr);

        int col = 0;
        int line = 0;
        for(int i = 0; i < font_bpg; i++) {
            int remain_width = font_width - col;
            if(remain_width > 8) remain_width = 8;
            for(int j = 0; j < remain_width; j++) {
                video_framebuffer_plot_pixel(
                    _cursor_posx * font_width + j + col,
                    _cursor_posy * font_height + line,
                    ((*(glyph+i) >> (7-j)) & 1) ? fg : bg
                );
            }

            col += remain_width;
            if(col >= font_width) {
                line++;
                col = 0;
            }
        }
        _cursor_posx++;
    }

    if(_cursor_posx == text_cols) {
        _cursor_posx = 0;
        _cursor_posy++;
    }
    if(move) {
        cursor_posx = _cursor_posx;
        cursor_posy = _cursor_posy;
        if(cursor_posy == text_rows)
            video_framebuffer_scroll_screen(1);
    }

    _framebuffer_draw_cursor(load_cursor_buffer);
}
