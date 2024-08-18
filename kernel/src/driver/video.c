#include "video.h"
#include "system.h"

#include "string.h"

// actually define the pointers else we would get undefined reference error
int  (*video_rgb)(int r, int g, int b);
int  (*video_get_cursor)();
void (*video_set_cursor)(int offset);
void (*video_cls)(int color);
void (*video_scroll_screen)(unsigned ammount);
void (*video_print_char)(char chr, int offset, int fg, int bg, bool move);

static uint8_t* vid_mem;

static bool linear_graphics_mode = false;

static int current_fg;
static int current_bg;

static int text_rows;
static int text_cols;

// mode-independent functions
void video_set_attr(int fg, int bg) {
    if(fg > -1) current_fg = fg;
    if(bg > -1) current_bg = bg;
}
void video_set_vidmem_ptr(uint32_t ptr) {
    vid_mem = (uint8_t*)ptr;
}
void video_size(int* w, int* h) {
    *w = text_cols;
    *h = text_rows;
}

// mode-dependent functions

// text mode

// TODO: implement textmode rgb
int video_textmode_rgb(int r, int g, int b) {
    if(r == 0 && g == 0 && b == 0)
        return 0x0;
    return 0x7;
}

void video_textmode_enable_cursor(int cursor_scanline_start, int cursor_scanline_end) {
    port_outb(PORT_SCREEN_CTRL, 0x0a);
    port_outb(PORT_SCREEN_DATA, (port_inb(PORT_SCREEN_DATA) & 0xc0) | cursor_scanline_start);

    port_outb(PORT_SCREEN_CTRL, 0x0b);
    port_outb(PORT_SCREEN_DATA, (port_inb(PORT_SCREEN_DATA) & 0xe0) | cursor_scanline_end);
}
void video_textmode_disable_cursor() {
    port_outb(PORT_SCREEN_CTRL, 0x0a);
    port_outb(PORT_SCREEN_DATA, 0x20);
}
int video_textmode_get_cursor() {
    int offset = 0;
    port_outb(PORT_SCREEN_CTRL, 0x0f);
    offset |= port_inb(PORT_SCREEN_DATA);
    port_outb(PORT_SCREEN_CTRL, 0x0e);
    offset |= port_inb(PORT_SCREEN_DATA) << 8;
    return offset;
}
void video_textmode_set_cursor(int offset) {
    port_outb(PORT_SCREEN_CTRL, 14);
    port_outb(PORT_SCREEN_DATA, (uint8_t)(offset >> 8));
    port_outb(PORT_SCREEN_CTRL, 15);
    port_outb(PORT_SCREEN_DATA, (uint8_t)(offset));
}
void video_textmode_cls(int bg) {
    for(int i = 0; i < text_rows * text_cols; i++) {
        vid_mem[i * 2 + 1] = (bg & 0xf) << 4;
        vid_mem[i * 2]     = ' ';
    }
    video_textmode_set_cursor(0);
}

void video_textmode_scroll_screen(unsigned ammount) {
    int end = video_textmode_get_cursor();

    int start = end - text_cols * ammount;
    // already on top
    if(end < text_cols) start = 0;

    for(int i = 0; i < start; i++) {
        vid_mem[i * 2] = vid_mem[i * 2 + text_cols * 2 * ammount];
        vid_mem[i * 2 + 1] = vid_mem[i * 2 + text_cols * 2 * ammount + 1];
    }
    for(int i = start; i <= end; i++) {
        vid_mem[i * 2] = ' ';
        vid_mem[i * 2 + 1] = 0xf;
    }

    video_textmode_set_cursor(start);
}

void video_textmode_print_char(char chr, int offset, int fg, int bg, bool move) {
    if(chr == 0) return;

    if(offset < 0) offset = video_textmode_get_cursor();
    if(fg < 0) fg = current_fg;
    if(bg < 0) bg = current_bg;

    if(chr == '\n') {
        offset += text_cols - (offset % text_cols);
    }
    else if(chr == '\b') {
        offset--;
        vid_mem[offset * 2] = ' ';
    }
    else if(chr == '\t') {
        // handle tabs like spaces
        // change later
        video_textmode_print_char(' ', offset, fg, bg, move);
        // offset += - offset % text_cols % 8 + 8;
    }
    else {
        vid_mem[offset * 2] = chr;
        vid_mem[offset * 2 + 1] = ((bg & 0xf) << 4) | (fg & 0xf);
        offset++;
    }

    if(move) {
        video_textmode_set_cursor(offset);
        if(offset > text_rows * text_cols - 1) video_textmode_scroll_screen(1);
    }
}

// linear graphics mode

#include "misc/psf.h"

static uint32_t framebuffer_pitch;
static uint32_t framebuffer_width;
static uint32_t framebuffer_height;
static uint8_t framebuffer_bpp;

static int cursor_posx = 0;
static int cursor_posy = 0;

static int font_width;
static int font_height;
static int font_bpg;

// the pixels behind the cursor
static int cursor_buffer[512];
static int cursor_buffer_posx = 0;
static int cursor_buffer_posy = 0;

int video_framebuffer_rgb(int r, int g, int b) {
    // TODO: support other bpp mode
    return (r << 16) | (g << 8) | b;
}

void video_framebuffer_plot_pixel(int x, int y, int color) {
    if(framebuffer_bpp == 8) {
        uint8_t* pixel = vid_mem + y * framebuffer_pitch + x;
        *pixel = color;
    }
    else if(framebuffer_bpp == 16) {
        uint16_t* pixel = (uint16_t*)(vid_mem + y * framebuffer_pitch) + x;
        *pixel = color;
    }
    else if(framebuffer_bpp == 32) {
        uint32_t* pixel = (uint32_t*)(vid_mem + y * framebuffer_pitch) + x;
        *pixel = color;
    }
    else {
        // who the hell use this is not deserved a pixel
    }
}

int video_framebuffer_get_pixel(int x, int y) {
    if(framebuffer_bpp == 8) {
        uint8_t* pixel = vid_mem + y * framebuffer_pitch + x;
        return *pixel;
    }
    else if(framebuffer_bpp == 16) {
        uint16_t* pixel = (uint16_t*)(vid_mem + y * framebuffer_pitch) + x;
        return *pixel;
    }
    else if(framebuffer_bpp == 32) {
        uint32_t* pixel = (uint32_t*)(vid_mem + y * framebuffer_pitch) + x;
         return *pixel;
    }
    else {
        // who the hell use this is not deserved a pixel
        return 0x0;
    }
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
    for(unsigned y = 0; y < framebuffer_height; y++)
        for(unsigned x = 0; x < framebuffer_width; x++)
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
        (char*)vid_mem,
        (char*)vid_mem + ammount * font_height * framebuffer_pitch,
        cursor_posy * font_height * framebuffer_pitch
    );
    memset(
        (char*)vid_mem + cursor_posy * font_height * framebuffer_pitch,
        0, ammount * font_height * framebuffer_pitch
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

void video_textmode_init(uint8_t cols, uint8_t rows) {
    linear_graphics_mode = false;
    current_fg = video_textmode_rgb(255, 255, 255);
    current_bg = video_textmode_rgb(0, 0, 0);

    video_rgb            = video_textmode_rgb;
    video_get_cursor     = video_textmode_get_cursor;
    video_set_cursor     = video_textmode_set_cursor;
    video_cls            = video_textmode_cls;
    video_print_char     = video_textmode_print_char;
    video_scroll_screen  = video_textmode_scroll_screen;

    text_cols = cols;
    text_rows = rows;
}
void video_framebuffer_init(uint32_t pitch, uint32_t width, uint32_t height, uint8_t bpp) {
    linear_graphics_mode = true;
    current_fg = video_framebuffer_rgb(255, 255, 255);
    current_bg = video_framebuffer_rgb(0, 0, 0);

    video_rgb            = video_framebuffer_rgb;
    video_get_cursor     = video_framebuffer_get_cursor;
    video_set_cursor     = video_framebuffer_set_cursor;
    video_cls            = video_framebuffer_cls;
    video_print_char     = video_framebuffer_print_char;
    video_scroll_screen  = video_framebuffer_scroll_screen;

    framebuffer_pitch = pitch;
    framebuffer_width = width;
    framebuffer_height = height;
    framebuffer_bpp = bpp;

    psf_get_font_geometry(&font_width, &font_height, &font_bpg);
    text_cols = framebuffer_width / font_width;
    text_rows = framebuffer_height / font_height;
}
