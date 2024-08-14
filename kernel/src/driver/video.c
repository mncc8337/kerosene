#include "video.h"
#include "system.h"

#ifdef __VIDEO_TEXT_MODE

static unsigned char* vid_mem = (unsigned char*)0xb8000;

static unsigned char current_attr = TTY_LIGHT_GREY;

void video_set_attr(unsigned char attr) {
    current_attr = attr;
}
void video_enable_cursor(unsigned char cursor_scanline_start, unsigned char cursor_scanline_end) {
	port_outb(PORT_SCREEN_CTRL, 0x0a);
	port_outb(PORT_SCREEN_DATA, (port_inb(PORT_SCREEN_DATA) & 0xc0) | cursor_scanline_start);

	port_outb(PORT_SCREEN_CTRL, 0x0b);
	port_outb(PORT_SCREEN_DATA, (port_inb(PORT_SCREEN_DATA) & 0xe0) | cursor_scanline_end);
}
void video_disable_cursor() {
	port_outb(PORT_SCREEN_CTRL, 0x0a);
	port_outb(PORT_SCREEN_DATA, 0x20);
}
int video_get_cursor() {
    int offset = 0;
    port_outb(PORT_SCREEN_CTRL, 0x0f);
    offset |= port_inb(PORT_SCREEN_DATA);
    port_outb(PORT_SCREEN_CTRL, 0x0e);
    offset |= port_inb(PORT_SCREEN_DATA) << 8;
    return offset;
}
void video_set_cursor(int offset){
    port_outb(PORT_SCREEN_CTRL, 14);
    port_outb(PORT_SCREEN_DATA, (unsigned char)(offset >> 8));
    port_outb(PORT_SCREEN_CTRL, 15);
    port_outb(PORT_SCREEN_DATA, (unsigned char)(offset));
}
void video_cls(char attr) {
    for(int i = 0; i < 80 * 25; i++) {
        vid_mem[i * 2 + 1] = attr;
        vid_mem[i * 2]     = ' ';
    }
    video_set_cursor(0);
}

static int _print_char(char chr, int offset, char attr) {
    if(chr == '\n') {
        offset += TTY_MAX_COLS - (offset % TTY_MAX_COLS);
    }
    else if(chr == '\b') {
        offset--;
        vid_mem[offset * 2] = ' ';
    }
    else if(chr == '\t') {
        // handle tabs like spaces
        // change later
        return _print_char(' ', offset, attr);
        // offset += - offset % TTY_MAX_COLS % 8 + 8;
    }
    else if(chr >= 0x20 && chr <= 0x7e) {
        vid_mem[offset * 2] = chr;
        vid_mem[offset * 2 + 1] = attr;
        offset++;
    }
    return offset;
}
void video_print_char(char chr, int offset, char attr, bool move) {
    if(offset < 0) offset = video_get_cursor();
    if(attr == 0) attr = current_attr;

    offset = _print_char(chr, offset, attr);

    if(move) {
        video_set_cursor(offset);
        if(offset > TTY_MAX_ROWS * TTY_MAX_COLS - 1) video_scroll_screen(1);
    }
}
void video_print_string(char* string, int offset, char attr, bool move) {
    if(offset < 0) offset = video_get_cursor();
    if(attr == 0) attr = current_attr;

    int i = 0;
    while(string[i] != '\0') {
        offset = _print_char(string[i], offset, attr);
        i++;
    }

    if(move) {
        video_set_cursor(offset);
        if(offset > TTY_MAX_ROWS * TTY_MAX_COLS - 1)
            video_scroll_screen((offset + (TTY_MAX_COLS - offset % TTY_MAX_COLS))/TTY_MAX_COLS - TTY_MAX_ROWS);
    }
}
void video_scroll_screen(unsigned int ammount) {
    int end = video_get_cursor();

    int start = end - TTY_MAX_COLS * ammount;
    // already on top
    if(end < TTY_MAX_COLS) start = 0;

    for(int i = 0; i < start; i++) {
        vid_mem[i * 2] = vid_mem[i * 2 + TTY_MAX_COLS * 2 * ammount];
        vid_mem[i * 2 + 1] = vid_mem[i * 2 + TTY_MAX_COLS * 2 * ammount + 1];
    }
    for(int i = start; i <= end; i++) {
        vid_mem[i * 2] = ' ';
        vid_mem[i * 2 + 1] = 0x0f;
    }

    video_set_cursor(start);
}

void video_set_vidmem_ptr(uint32_t ptr) {
    vid_mem = (unsigned char*)ptr;
}

#else

static unsigned char* vid_mem = (unsigned char*)0xb8000;

static unsigned char current_attr = TTY_LIGHT_GREY;

void video_set_attr(unsigned char attr) {}
void video_enable_cursor(unsigned char cursor_scanline_start, unsigned char cursor_scanline_end) {}
void video_disable_cursor() {}
int video_get_cursor() {}
void video_set_cursor(int offset) {}
void video_cls(char attr) {}
static int _print_char(char chr, int offset, char attr) {}
void video_print_char(char chr, int offset, char attr, bool move) {}
void video_print_string(char* string, int offset, char attr, bool move) {}
void video_scroll_screen(unsigned int ammount) {}
void video_set_vidmem_ptr(uint32_t ptr) {}

#endif
