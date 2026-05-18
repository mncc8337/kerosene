#include <video.h>
#include <system.h>

#include <string.h>

typedef struct {
    uint8_t vga_val, r, g, b;
} vga_color_t;

static const vga_color_t vga_palette[] = {
    {0, 0, 0, 0},
    {1, 0, 0, 170},
    {2, 0, 170, 0},
    {3, 0, 170, 170},
    {4, 170, 0, 0},
    {5, 170, 0, 170},
    {6, 170, 85, 0},
    {7, 170, 170, 170},
    {8, 85, 85, 85},
    {9, 85, 85, 255},
    {10, 85, 255, 85},
    {11, 85, 255, 255},
    {12, 255, 85, 85},
    {13, 255, 85, 255},
    {14, 255, 255, 85},
    {15, 255, 255, 255}
};

static uint8_t* vid_mem = (uint8_t*)VIDEO_START;

static uint8_t current_attr = 0x7;

static int text_rows;
static int text_cols;

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
    int best_color = 7;
    unsigned min_diff = 0xFFFFFFFF;

    for (int i = 0; i < 16; i++) {
        unsigned diff = color_difference(
            r, g, b,
            vga_palette[i].r,
            vga_palette[i].g,
            vga_palette[i].b
        );
        if(diff < min_diff) {
            min_diff = diff;
            best_color = vga_palette[i].vga_val;
        }
    }
    return best_color;
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
    if((int)ammount >= text_rows) {
        video_vga_cls(current_attr >> 4);
        return;
    }

    uint16_t* vga_buffer = (uint16_t*)vid_mem;
    int total_cells = text_rows * text_cols;
    int shift_cells = ammount * text_cols;

    memmove(vga_buffer, vga_buffer + shift_cells, (total_cells - shift_cells) * 2);

    uint16_t clear_val = (uint16_t)(current_attr << 8) | ' ';
    for (int i = total_cells - shift_cells; i < total_cells; i++) {
        vga_buffer[i] = clear_val;
    }

    int current_pos = video_vga_get_cursor();
    video_vga_set_cursor(current_pos < shift_cells ? 0 : current_pos - shift_cells);
}

static void printc(char chr, int* offset_ptr, uint8_t attr, bool calculate) {
    int offset = *offset_ptr;

    if(chr == '\n') {
        offset += text_cols - (offset % text_cols);
    } else if(chr == '\b') {
        if(offset > 0) offset--;
        if(!calculate) {
            vid_mem[offset * 2] = ' ';
        }
    } else {
        if(!calculate) {
            vid_mem[offset * 2] = chr;
            vid_mem[offset * 2 + 1] = attr;
        }
        offset++;
    }

    *offset_ptr = offset;
}

void video_vga_printc(char chr, int offset, int fg, int bg, bool move) {
    if(chr == 0) return;

    if(offset < 0) offset = video_vga_get_cursor();

    uint8_t attr = current_attr;
    if(fg >= 0) attr = (attr & 0x0f) | fg;
    if(bg >= 0) attr = (attr & 0xf0) | bg;

    printc(chr, &offset, attr, false);

    if(move) {
        video_vga_set_cursor(offset);
        if(offset > text_rows * text_cols - 1) scroll_screen(1);
    }
}

void video_vga_prints(const char* str, int offset, int fg, int bg, bool move) {
    if(*str == 0) return;

    if(offset < 0) offset = video_vga_get_cursor();

    uint8_t attr = current_attr;
    if(fg >= 0) attr = (attr & 0xf0) | (fg & 0x0f);
    if(bg >= 0) attr = (attr & 0x0f) | ((bg & 0x0f) << 4);

    if(move) {
        int phantom = offset;
        for(const char* p = str; *p; p++) {
            printc(*p, &phantom, attr, true);
        }

        if(phantom >= text_rows * text_cols) {
            int rows_needed = (phantom / text_cols) - text_rows + 1;
            scroll_screen(rows_needed);
            
            offset -= rows_needed * text_cols;
            if(offset < 0) offset = 0;
        }
    }

    const int screen_chars = text_rows * text_cols;
    for(const char* p = str; *p; p++) {
        if(offset >= screen_chars) break;
        printc(*p, &offset, attr, false);
    }

    if(move) {
        video_vga_set_cursor(offset);
    }
}
