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
void print_char(char chr, int offset, char attr, bool move) {
    if(offset < 0) offset = get_cursor();

    if(chr == '\n') {
        int t = offset % MAX_COLS;
        offset += MAX_COLS - t;
    }
    else {
        vid_mem[offset * 2]     = chr;
        if(attr != 0) vid_mem[offset * 2 + 1] = attr;
        offset++;
    }

    if(move) {
        set_cursor(offset);
        if(offset > MAX_ROWS * MAX_COLS - 1) scroll_screen();
    }
}
void print_string(char* string, int offset, char attr, bool move) {
    if(offset < 0) offset = get_cursor();
    if(attr == 0) attr = 0x0f;

    while(*string != '\0') {
        if(*string == '\n') {
            int t = offset % MAX_COLS;
            offset += MAX_COLS - t;
        }
        else {
            vid_mem[offset * 2]     = *string;
            if(attr != 0) vid_mem[offset * 2 + 1] = attr;
            offset++;
        }
        string++;
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
