#include "driver/video.h"

unsigned char* vid_mem = (unsigned char*)0xb8000;

int get_attr(int bg, int fg) {
    return bg * 16 + fg;
}
int get_offset(int row, int col) {
    return row * MAX_COLS + col;
}

void enable_cursor(unsigned char cursor_scanline_start, unsigned char cursor_scanline_end) {
	port_outb(PORT_SCREEN_CTR, 0x0a);
	port_outb(PORT_SCREEN_DAT, (port_inb(PORT_SCREEN_DAT) & 0xc0) | cursor_scanline_start);

	port_outb(PORT_SCREEN_CTR, 0x0b);
	port_outb(PORT_SCREEN_DAT, (port_inb(PORT_SCREEN_DAT) & 0xe0) | cursor_scanline_end);
}
void disable_cursor() {
	port_outb(PORT_SCREEN_CTR, 0x0a);
	port_outb(PORT_SCREEN_DAT, 0x20);
}
int get_cursor() {
    int offset = 0;
    port_outb(PORT_SCREEN_CTR, 0x0f);
    offset |= port_inb(PORT_SCREEN_DAT);
    port_outb(PORT_SCREEN_CTR, 0x0e);
    offset |= port_inb(PORT_SCREEN_DAT) << 8;
    return offset;
}
void set_cursor(int offset){
    port_outb(PORT_SCREEN_CTR, 14);
    port_outb(PORT_SCREEN_DAT, (unsigned char)(offset >> 8));
    port_outb(PORT_SCREEN_CTR, 15);
    port_outb(PORT_SCREEN_DAT, (unsigned char)(offset));
}
void cls(char attr) {
    for(int i = 0; i < 80 * 25; i++) {
        vid_mem[i * 2 + 1] = attr;
        vid_mem[i * 2]     = ' ';
    }
    set_cursor(0);
}

int _print_char(char chr, int offset, char attr) {
    if(chr == '\n') {
        offset += MAX_COLS - (offset % MAX_COLS);
    }
    else if(chr == '\b') {
        offset--;
        vid_mem[offset * 2] = ' ';
    }
    else if(chr == '\t') {
        // handle tabs like spaces
        // change later
        return _print_char(' ', offset, attr);
        // offset += - offset % MAX_COLS % 8 + 8;
    }
    else if(chr >= 0x20 && chr <= 0x7e) {
        vid_mem[offset * 2] = chr;
        if(attr != 0) vid_mem[offset * 2 + 1] = attr;
        offset++;
    }
    return offset;
}
void print_char(char chr, int offset, char attr, bool move) {
    if(offset < 0) offset = get_cursor();

    offset = _print_char(chr, offset, attr);

    if(move) {
        set_cursor(offset);
        if(offset > MAX_ROWS * MAX_COLS - 1) scroll_screen();
    }
}
void print_string(char* string, int offset, char attr, bool move) {
    if(offset < 0) offset = get_cursor();

    int i = 0;
    while(string[i] != '\0') {
        offset = _print_char(string[i], offset, attr);
        i++;
    }

    if(move) {
        set_cursor(offset);
        if(offset > MAX_ROWS * MAX_COLS - 1) scroll_screen();
    }
}
void scroll_screen() {
    int end = get_cursor();

    // already on top
    if(end < MAX_COLS) return;

    int start = end - MAX_COLS;

    for(int i = 0; i < start; i++) {
        vid_mem[i * 2] = vid_mem[i * 2 + MAX_COLS * 2];
        vid_mem[i * 2 + 1] = vid_mem[i * 2 + MAX_COLS * 2 + 1];
    }
    for(int i = start; i <= end; i++) {
        vid_mem[i * 2] = ' ';
        vid_mem[i * 2 + 1] = 0x0f;
    }

    set_cursor(start);
}
