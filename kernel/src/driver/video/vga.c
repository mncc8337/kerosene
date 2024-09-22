#include "video.h"
#include "system.h"

static uint8_t* vid_mem;

static uint8_t current_attr = 0x7;

static int text_rows;
static int text_cols;

void video_vga_set_ptr(int ptr) {
    vid_mem = (uint8_t*)ptr;
}

void video_vga_set_attr(int fg, int bg) {
    current_attr = (bg << 4) | fg;
}

void video_vga_set_size(int w, int h) {
    text_cols = w;
    text_rows = h;
}

void video_vga_get_size(int* w, int* h) {
    *w = text_cols;
    *h = text_rows;
}

void video_vga_enable_cursor(int cursor_scanline_start, int cursor_scanline_end) {
    port_outb(PORT_SCREEN_CTRL, 0x0a);
    port_outb(PORT_SCREEN_DATA, (port_inb(PORT_SCREEN_DATA) & 0xc0) | cursor_scanline_start);

    port_outb(PORT_SCREEN_CTRL, 0x0b);
    port_outb(PORT_SCREEN_DATA, (port_inb(PORT_SCREEN_DATA) & 0xe0) | cursor_scanline_end);
}

void video_vga_disable_cursor() {
    port_outb(PORT_SCREEN_CTRL, 0x0a);
    port_outb(PORT_SCREEN_DATA, 0x20);
}

static unsigned color_difference(unsigned r1, unsigned g1, unsigned b1, unsigned r2, unsigned g2, unsigned b2) {
    signed r = r1 - r2;
    signed g = g1 - g2;
    signed b = b1 - b2;

    return r*r + g*g + b*b;
}

int video_vga_rgb(int r, int g, int b) {
    int ret = 0x7;
    unsigned color_diff;
    unsigned _color_diff;

    // is there a much more elegant way to do this lol

    _color_diff = color_difference(r, g, b, VIDEO_BLACK);
    color_diff = _color_diff;
    ret = VIDEO_VGA_BLACK;

    _color_diff = color_difference(r, g, b, VIDEO_BLUE);
    if(_color_diff < color_diff) {
        color_diff = _color_diff;
        ret = VIDEO_VGA_BLUE;
    }

    _color_diff = color_difference(r, g, b, VIDEO_GREEN);
    if(_color_diff < color_diff) {
        color_diff = _color_diff;
        ret = VIDEO_VGA_GREEN;
    }

    _color_diff = color_difference(r, g, b, VIDEO_CYAN);
    if(_color_diff < color_diff) {
        color_diff = _color_diff;
        ret = VIDEO_VGA_CYAN;
    }

    _color_diff = color_difference(r, g, b, VIDEO_RED);
    if(_color_diff < color_diff) {
        color_diff = _color_diff;
        ret = VIDEO_VGA_RED;
    }

    _color_diff = color_difference(r, g, b, VIDEO_MAGENTA);
    if(_color_diff < color_diff) {
        color_diff = _color_diff;
        ret = VIDEO_VGA_MAGENTA;
    }

    _color_diff = color_difference(r, g, b, VIDEO_YELLOW);
    if(_color_diff < color_diff) {
        color_diff = _color_diff;
        ret = VIDEO_VGA_YELLOW;
    }

    _color_diff = color_difference(r, g, b, VIDEO_LIGHT_GREY);
    if(_color_diff < color_diff) {
        color_diff = _color_diff;
        ret = VIDEO_VGA_LIGHT_GREY;
    }

    _color_diff = color_difference(r, g, b, VIDEO_DARK_GREY);
    if(_color_diff < color_diff) {
        color_diff = _color_diff;
        ret = VIDEO_VGA_DARK_GREY;
    }

    _color_diff = color_difference(r, g, b, VIDEO_LIGHT_BLUE);
    if(_color_diff < color_diff) {
        color_diff = _color_diff;
        ret = VIDEO_VGA_LIGHT_BLUE;
    }

    _color_diff = color_difference(r, g, b, VIDEO_LIGHT_GREEN);
    if(_color_diff < color_diff) {
        color_diff = _color_diff;
        ret = VIDEO_VGA_LIGHT_GREEN;
    }

    _color_diff = color_difference(r, g, b, VIDEO_LIGHT_CYAN);
    if(_color_diff < color_diff) {
        color_diff = _color_diff;
        ret = VIDEO_VGA_LIGHT_CYAN;
    }

    _color_diff = color_difference(r, g, b, VIDEO_LIGHT_RED);
    if(_color_diff < color_diff) {
        color_diff = _color_diff;
        ret = VIDEO_VGA_LIGHT_RED;
    }

    _color_diff = color_difference(r, g, b, VIDEO_LIGHT_MAGENTA);
    if(_color_diff < color_diff) {
        color_diff = _color_diff;
        ret = VIDEO_VGA_LIGHT_MAGENTA;
    }

    _color_diff = color_difference(r, g, b, VIDEO_LIGHT_YELLOW);
    if(_color_diff < color_diff) {
        color_diff = _color_diff;
        ret = VIDEO_VGA_LIGHT_YELLOW;
    }

    _color_diff = color_difference(r, g, b, VIDEO_WHITE);
    if(_color_diff < color_diff) {
        color_diff = _color_diff;
        ret = VIDEO_VGA_WHITE;
    }

    return ret;
}

int video_vga_get_cursor() {
    int offset = 0;
    port_outb(PORT_SCREEN_CTRL, 0x0f);
    offset |= port_inb(PORT_SCREEN_DATA);
    port_outb(PORT_SCREEN_CTRL, 0x0e);
    offset |= port_inb(PORT_SCREEN_DATA) << 8;
    return offset;
}

void video_vga_set_cursor(int offset) {
    port_outb(PORT_SCREEN_CTRL, 14);
    port_outb(PORT_SCREEN_DATA, (uint8_t)(offset >> 8));
    port_outb(PORT_SCREEN_CTRL, 15);
    port_outb(PORT_SCREEN_DATA, (uint8_t)(offset));
}

void video_vga_cls(int bg) {
    for(int i = 0; i < text_rows * text_cols; i++) {
        vid_mem[i * 2 + 1] = (bg & 0xf) << 4;
        vid_mem[i * 2]     = ' ';
    }
    video_vga_set_cursor(0);
}

static void scroll_screen(unsigned ammount) {
    int end = video_vga_get_cursor();

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

    video_vga_set_cursor(start);
}

void video_vga_print_char(char chr, int offset, int fg, int bg, bool move) {
    if(chr == 0) return;

    if(offset < 0) offset = video_vga_get_cursor();

    uint8_t attr = current_attr;
    if(fg >= 0) attr = (attr & 0x0f) | fg;
    if(bg >= 0) attr = (attr & 0xf0) | bg;

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
        video_vga_print_char(' ', offset, fg, bg, move);
        // offset += - offset % text_cols % 8 + 8;
    }
    else {
        vid_mem[offset * 2] = chr;
        vid_mem[offset * 2 + 1] = attr;
        offset++;
    }

    if(move) {
        video_vga_set_cursor(offset);
        if(offset > text_rows * text_cols - 1) scroll_screen(1);
    }
}
