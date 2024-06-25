#include "system.h"
#include "video.h"

unsigned char* vid_mem;

int get_offset(int row, int col) {
    return row * MAX_COLS + col;
}

void enable_cursor(unsigned char cursor_scanline_start, unsigned char cursor_scanline_end) {
	port_outb(REG_SCREEN_CTRL, 0x0a);
	port_outb(REG_SCREEN_DATA, (port_inb(REG_SCREEN_DATA) & 0xc0) | cursor_scanline_start);

	port_outb(REG_SCREEN_CTRL, 0x0b);
	port_outb(REG_SCREEN_DATA, (port_inb(REG_SCREEN_DATA) & 0xe0) | cursor_scanline_end);
}
void disable_cursor() {
	port_outb(REG_SCREEN_CTRL, 0x0a);
	port_outb(REG_SCREEN_DATA, 0x20);
}
int get_cursor() {
    int offset = 0;
    port_outb(REG_SCREEN_CTRL, 0x0f);
    offset |= port_inb(REG_SCREEN_DATA);
    port_outb(REG_SCREEN_CTRL, 0x0e);
    offset |= port_inb(REG_SCREEN_DATA) << 8;
    return offset;
}
void set_cursor(int offset){
    port_outb(REG_SCREEN_CTRL, 14);
    port_outb(REG_SCREEN_DATA, (unsigned char)(offset >> 8));
    port_outb(REG_SCREEN_CTRL, 15);
    port_outb(REG_SCREEN_DATA, (unsigned char)(offset));
}
void cls(char attr) {
    for(int i = 0; i < 80 * 25; i++) {
        vid_mem[i * 2 + 1] = attr;
        vid_mem[i * 2]     = ' ';
    }
    set_cursor(0);
    return;
}
void print_char(char chr, int offset, char attr) {
    if(offset < 0) offset = get_cursor();
    if(attr == 0) attr = 0x0f;

    if(chr == '\n') {
        int t = offset % MAX_COLS;
        offset = MAX_COLS - t + offset;
    }
    else {
        vid_mem[offset * 2]     = chr;
        vid_mem[offset * 2 + 1] = attr;
        offset++;
    }
    set_cursor(offset);
    return;
}
void print_string(char* string, int offset, char attr) {
    if(offset < 0) offset = get_cursor();
    if(attr == 0) attr = 0x0f;

    while(*string != '\0') {
        if(*string == '\n') {
            int t = offset % MAX_COLS;
            offset += MAX_COLS - t;
        }
        else {
            vid_mem[offset * 2]     = *string;
            vid_mem[offset * 2 + 1] = attr;
            offset++;
        }
        string++;
    }
    set_cursor(offset);
    return;
}
void init_video() {
    vid_mem = (unsigned char*) VIDEO_ADDRESS;
    return;
}
