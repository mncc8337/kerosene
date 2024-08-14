#include "video.h"
#include "system.h"

static uint8_t* vid_mem;

static uint8_t current_attr = VIDEO_LIGHT_GREY;

// mode-independent functions
void video_set_attr(uint8_t attr) {
    current_attr = attr;
}
void video_set_vidmem_ptr(uint32_t ptr) {
    vid_mem = (uint8_t*)ptr;
}

// mode-dependent functions

// text mode

void video_textmode_enable_cursor(uint8_t cursor_scanline_start, uint8_t cursor_scanline_end) {
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
void video_textmode_cls(char attr) {
    for(int i = 0; i < 80 * 25; i++) {
        vid_mem[i * 2 + 1] = attr;
        vid_mem[i * 2]     = ' ';
    }
    video_textmode_set_cursor(0);
}

void video_textmode_scroll_screen(unsigned ammount) {
    int end = video_textmode_get_cursor();

    int start = end - VIDEO_MAX_COLS * ammount;
    // already on top
    if(end < VIDEO_MAX_COLS) start = 0;

    for(int i = 0; i < start; i++) {
        vid_mem[i * 2] = vid_mem[i * 2 + VIDEO_MAX_COLS * 2 * ammount];
        vid_mem[i * 2 + 1] = vid_mem[i * 2 + VIDEO_MAX_COLS * 2 * ammount + 1];
    }
    for(int i = start; i <= end; i++) {
        vid_mem[i * 2] = ' ';
        vid_mem[i * 2 + 1] = 0x0f;
    }

    video_textmode_set_cursor(start);
}

static int _print_char(char chr, int offset, char attr) {
    if(chr == '\n') {
        offset += VIDEO_MAX_COLS - (offset % VIDEO_MAX_COLS);
    }
    else if(chr == '\b') {
        offset--;
        vid_mem[offset * 2] = ' ';
    }
    else if(chr == '\t') {
        // handle tabs like spaces
        // change later
        return _print_char(' ', offset, attr);
        // offset += - offset % VIDEO_MAX_COLS % 8 + 8;
    }
    else if(chr >= 0x20 && chr <= 0x7e) {
        vid_mem[offset * 2] = chr;
        vid_mem[offset * 2 + 1] = attr;
        offset++;
    }
    return offset;
}
void video_textmode_print_char(char chr, int offset, char attr, bool move) {
    if(offset < 0) offset = video_textmode_get_cursor();
    if(attr == 0) attr = current_attr;

    offset = _print_char(chr, offset, attr);

    if(move) {
        video_textmode_set_cursor(offset);
        if(offset > VIDEO_MAX_ROWS * VIDEO_MAX_COLS - 1) video_textmode_scroll_screen(1);
    }
}
void video_textmode_print_string(char* string, int offset, char attr, bool move) {
    if(offset < 0) offset = video_textmode_get_cursor();
    if(attr == 0) attr = current_attr;

    int i = 0;
    while(string[i] != '\0') {
        offset = _print_char(string[i], offset, attr);
        i++;
    }

    if(move) {
        video_textmode_set_cursor(offset);
        if(offset > VIDEO_MAX_ROWS * VIDEO_MAX_COLS - 1)
            video_textmode_scroll_screen((offset + (VIDEO_MAX_COLS - offset % VIDEO_MAX_COLS))/VIDEO_MAX_COLS - VIDEO_MAX_ROWS);
    }
}

// linear graphics mode

static uint32_t framebuffer_pitch;
static uint32_t framebuffer_width;
static uint32_t framebuffer_height;
static uint8_t framebuffer_bpp;

static bool cursor_enable = true;
static uint8_t scanline_start = 0;
static uint8_t scanline_end = 15;
static int cursor_pos = 0;

void video_framebuffer_enable_cursor(uint8_t start, uint8_t end) {
    scanline_start = start;
    scanline_end = end;
    cursor_enable = true;
}
void video_framebuffer_disable_cursor() {
    cursor_enable = false;
}

int video_framebuffer_get_cursor() {
    return cursor_pos;
}
void video_framebuffer_set_cursor(int offset) {
    cursor_pos = offset;
}

void video_framebuffer_cls(char attr) {}

void video_framebuffer_print_char(char chr, int offset, char attr, bool move) {}
void video_framebuffer_print_string(char* string, int offset, char attr, bool move) {}
void video_framebuffer_scroll_screen(unsigned int ammount) {}

// linear graphics mode special functions

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
        // NOTE: not tested!
        int col_offset = x * framebuffer_bpp;
        uint8_t* pixel = vid_mem + y * framebuffer_pitch + col_offset / 8;
        uint8_t offset = col_offset % 8;

        for(int i = framebuffer_bpp; i > 0; i -= 8) {
            *pixel |= (uint8_t)(color << offset);
            offset = (offset + 8) % 8;
            color >>= 8;
            pixel++;
        }
    }
}

// actually define the pointers else we would get undefined reference error
void (*video_enable_cursor)(uint8_t cursor_scanline_start, uint8_t cursor_scanline_end);
void (*video_disable_cursor)();
int  (*video_get_cursor)();
void (*video_set_cursor)(int offset);
void (*video_cls)(char attr);
void (*video_scroll_screen)(unsigned ammount);
void (*video_print_char)(char chr, int offset, char attr, bool move);
void (*video_print_string)(char* string, int offset, char attr, bool move);

void video_textmode_init() {
    video_enable_cursor  = video_textmode_enable_cursor;
    video_disable_cursor = video_textmode_disable_cursor;
    video_get_cursor     = video_textmode_get_cursor;
    video_set_cursor     = video_textmode_set_cursor;
    video_cls            = video_textmode_cls;
    video_print_char     = video_textmode_print_char;
    video_print_string   = video_textmode_print_string;
    video_scroll_screen  = video_textmode_scroll_screen;
}
void video_framebuffer_init(uint32_t pitch, uint32_t width, uint32_t height, uint8_t bpp) {
    video_enable_cursor  = video_framebuffer_enable_cursor;
    video_disable_cursor = video_framebuffer_disable_cursor;
    video_get_cursor     = video_framebuffer_get_cursor;
    video_set_cursor     = video_framebuffer_set_cursor;
    video_cls            = video_framebuffer_cls;
    video_print_char     = video_framebuffer_print_char;
    video_print_string   = video_framebuffer_print_string;
    video_scroll_screen  = video_framebuffer_scroll_screen;

    framebuffer_pitch = pitch;
    framebuffer_width = width;
    framebuffer_height = height;
    framebuffer_bpp = bpp;
}
