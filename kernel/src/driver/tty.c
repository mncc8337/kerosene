#include "tty.h"

static unsigned char* vid_mem = (unsigned char*)0xb8000;

static unsigned char current_attr = LIGHT_GREY;

void tty_set_attr(unsigned char attr) {
    current_attr = attr;
}
void tty_enable_cursor(unsigned char cursor_scanline_start, unsigned char cursor_scanline_end) {
	port_outb(PORT_SCREEN_CTRL, 0x0a);
	port_outb(PORT_SCREEN_DATA, (port_inb(PORT_SCREEN_DATA) & 0xc0) | cursor_scanline_start);

	port_outb(PORT_SCREEN_CTRL, 0x0b);
	port_outb(PORT_SCREEN_DATA, (port_inb(PORT_SCREEN_DATA) & 0xe0) | cursor_scanline_end);
}
void tty_disable_cursor() {
	port_outb(PORT_SCREEN_CTRL, 0x0a);
	port_outb(PORT_SCREEN_DATA, 0x20);
}
int tty_get_cursor() {
    int offset = 0;
    port_outb(PORT_SCREEN_CTRL, 0x0f);
    offset |= port_inb(PORT_SCREEN_DATA);
    port_outb(PORT_SCREEN_CTRL, 0x0e);
    offset |= port_inb(PORT_SCREEN_DATA) << 8;
    return offset;
}
void tty_set_cursor(int offset){
    port_outb(PORT_SCREEN_CTRL, 14);
    port_outb(PORT_SCREEN_DATA, (unsigned char)(offset >> 8));
    port_outb(PORT_SCREEN_CTRL, 15);
    port_outb(PORT_SCREEN_DATA, (unsigned char)(offset));
}
void tty_cls(char attr) {
    for(int i = 0; i < 80 * 25; i++) {
        vid_mem[i * 2 + 1] = attr;
        vid_mem[i * 2]     = ' ';
    }
    tty_set_cursor(0);
}

static int _print_char(char chr, int offset, char attr) {
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
        vid_mem[offset * 2 + 1] = attr;
        offset++;
    }
    return offset;
}
void tty_print_char(char chr, int offset, char attr, bool move) {
    if(offset < 0) offset = tty_get_cursor();
    if(attr == 0) attr = current_attr;

    offset = _print_char(chr, offset, attr);

    if(move) {
        tty_set_cursor(offset);
        if(offset > MAX_ROWS * MAX_COLS - 1) tty_scroll_screen(1);
    }
}
void tty_print_string(char* string, int offset, char attr, bool move) {
    if(offset < 0) offset = tty_get_cursor();
    if(attr == 0) attr = current_attr;

    int i = 0;
    while(string[i] != '\0') {
        offset = _print_char(string[i], offset, attr);
        i++;
    }

    if(move) {
        tty_set_cursor(offset);
        if(offset > MAX_ROWS * MAX_COLS - 1)
            tty_scroll_screen((offset + (MAX_COLS - offset % MAX_COLS))/MAX_COLS - MAX_ROWS);
    }
}
void tty_scroll_screen(unsigned int ammount) {
    int end = tty_get_cursor();

    int start = end - MAX_COLS * ammount;
    // already on top
    if(end < MAX_COLS) start = 0;

    for(int i = 0; i < start; i++) {
        vid_mem[i * 2] = vid_mem[i * 2 + MAX_COLS * 2 * ammount];
        vid_mem[i * 2 + 1] = vid_mem[i * 2 + MAX_COLS * 2 * ammount + 1];
    }
    for(int i = start; i <= end; i++) {
        vid_mem[i * 2] = ' ';
        vid_mem[i * 2 + 1] = 0x0f;
    }

    tty_set_cursor(start);
}

void tty_set_vidmem_ptr(uint32_t ptr) {
    vid_mem = (unsigned char*)ptr;
}
