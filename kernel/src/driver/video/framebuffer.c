#include <video.h>
#include <misc/psf.h>

#include <string.h>

static uint8_t* framebuffer = (uint8_t*)VIDEO_START;

static int current_fg = 0x969696;
static int current_bg = 0x000000;

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

static void scroll_screen(unsigned ammount) {
    if(ammount == 0) return;

    if((int)ammount >= text_rows) {
        memset(framebuffer, 0, text_rows * font_height * fb_pitch);
        cursor_posy = 0;
        cursor_buffer_posy = 0;
        return;
    }

    size_t bytes_per_row = font_height * fb_pitch;
    size_t bytes_to_move = (text_rows - ammount) * bytes_per_row;
    size_t offset_bytes = ammount * bytes_per_row;

    memmove(framebuffer, (uint8_t*)framebuffer + offset_bytes, bytes_to_move);
    memset((uint8_t*)framebuffer + bytes_to_move, 0, offset_bytes);

    if((int)cursor_posy >= (int)ammount) {
        cursor_posy -= ammount;
    } else {
        cursor_posy = 0;
    }

    if((int)cursor_buffer_posy >= (int)ammount) {
        cursor_buffer_posy -= ammount;
    } else {
        cursor_buffer_posy = 0;
    }
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

bool video_framebuffer_set_font(const char* font_data) {
    bool res = psf_load(font_data);
    if(res) return true;

    int font_width, font_height, font_bpg;
    psf_get_font_geometry(&font_width, &font_height, &font_bpg);
    video_framebuffer_set_font_size(font_width, font_height, font_bpg);

    cursor_posx = 0;
    cursor_posy = 0;

    return false;
}

void video_framebuffer_get_rowcol(int* c, int* r) {
    *c = text_cols;
    *r = text_rows;
}

void video_framebuffer_plot_pixel(unsigned x, unsigned y, int color) {
    // TODO: support other bpp

    if(x >= fb_width || y >= fb_height) return;

    if(fb_bpp == 8) {
        uint8_t* pixel = framebuffer + y * fb_pitch + x;
        *pixel = color;
    } else if(fb_bpp == 16) {
        uint16_t* pixel = (uint16_t*)(framebuffer + y * fb_pitch) + x;
        *pixel = color;
    } else if(fb_bpp == 32) {
        uint32_t* pixel = (uint32_t*)(framebuffer + y * fb_pitch) + x;
        *pixel = color;
    }
}

int video_framebuffer_get_pixel(unsigned x, unsigned y) {
    // TODO: support other bpp

    if(x >= fb_width || y >= fb_height) return 0;
    
    int ret;

    if(fb_bpp == 8) {
        uint8_t* pixel = framebuffer + y * fb_pitch + x;
        ret = *pixel;
    } else if(fb_bpp == 16) {
        uint16_t* pixel = (uint16_t*)(framebuffer + y * fb_pitch) + x;
        ret = *pixel;
    } else if(fb_bpp == 32) {
        uint32_t* pixel = (uint32_t*)(framebuffer + y * fb_pitch) + x;
        ret = *pixel;
    } else {
        ret = 0;
    }

    return ret;
}

void video_framebuffer_fill_rectangle(int x0, int y0, int x1, int y1, int color) {
    // TODO: support other bpp

    if((unsigned)x0 >= fb_width) x0 = fb_width-1;
    if((unsigned)x1 >= fb_width) x1 = fb_width-1;
    if((unsigned)y0 >= fb_height) y0 = fb_height-1;
    if((unsigned)y1 >= fb_height) y1 = fb_height-1;

    int dx = x1 - x0;
    if(dx < 0) {
        dx *= -1;
        int _x1 = x1;
        x1 = x0;
        x0 = _x1;
    }
    int dy = y1 - y0;
    if(dy < 0) {
        dy *= -1;
        int _y1 = y1;
        y1 = y0;
        y0 = _y1;
    }

    int bytes_per_pixel = fb_bpp / 8;
    uint8_t* fb = framebuffer;
    fb += y0 * fb_pitch + x0 * bytes_per_pixel;

    for(int y = y0; y <= y1; y++) {
        for(int x = x0; x <= x1; x++) {
            if(fb_bpp == 8) *fb = color;
            else if(fb_bpp == 16) *(uint16_t*)fb = color;
            else if(fb_bpp == 32) *(uint32_t*)fb = color;
            fb += bytes_per_pixel;
        }
        fb += fb_pitch - (dx+1) * bytes_per_pixel;
    }
}

static void draw_line_low(int x0, int y0, int x1, int y1, int color) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int yi = 1;

    if(dy < 0) {
        yi = -1;
        dy *= -1;
    }
    int D = (2 * dy) - dx;
    int y = y0;

    for(int x = x0; x <= x1; x++) {
        video_framebuffer_plot_pixel(x, y, color);
        if(D > 0) {
            y += yi;
            D += 2 * (dy - dx);
        } else {
            D += 2 * dy;
        }
    }
}

static void draw_line_high(int x0, int y0, int x1, int y1, int color) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int xi = 1;

    if(dx < 0) {
        xi = -1;
        dx *= -1;
    }
    int D = (2 * dx) - dy;
    int x = x0;

    for(int y = y0; y <= y1; y++) {
        video_framebuffer_plot_pixel(x, y, color);
        if(D > 0) {
            x += xi;
            D += 2 * (dx - dy);
        }
        else D += 2 * dx;
    }
}

void video_framebuffer_draw_line(int x0, int y0, int x1, int y1, int color) {
    int dy = y1 - y0;
    if(y1 < y0) dy = y0 - y1;

    int dx = x1 - x0;
    if(x1 < x0) dx = x0 - x1;

    if(dy < dx) {
        if(x0 > x1) draw_line_low(x1, y1, x0, y0, color);
        else draw_line_low(x0, y0, x1, y1, color);
    } else {
        if(y0 > y1) draw_line_high(x1, y1, x0, y0, color);
        else draw_line_high(x0, y0, x1, y1, color);
    }
}

void video_framebuffer_draw_circle(int x0, int y0, int r, int color) {
    int x = r;
    int y = 0;

    video_framebuffer_plot_pixel(x + x0, y + y0, color);

    video_framebuffer_plot_pixel(x + x0, -y + y0, color);
    video_framebuffer_plot_pixel(y + x0, x + y0 , color);
    video_framebuffer_plot_pixel(-y + x0, x + y0, color);

    int P = 1 - r;
    while(x > y) {
        y++;
        if(P <= 0) P += 2 * y + 1;
        else {
            x--;
            P += 2 * (y - x) + 1;
        }

        if(x < y) break;

        video_framebuffer_plot_pixel(x + x0, y + y0, color);
        video_framebuffer_plot_pixel(-x + x0, y + y0, color);
        video_framebuffer_plot_pixel(x + x0, -y + y0, color);
        video_framebuffer_plot_pixel(-x + x0, -y + y0, color);
        if(x != y) {
            video_framebuffer_plot_pixel(y + x0, x + y0, color);
            video_framebuffer_plot_pixel(-y + x0, x + y0, color);
            video_framebuffer_plot_pixel(y + x0, -x + y0, color);
            video_framebuffer_plot_pixel(-y + x0, -x + y0, color);
        }
    }
}

int video_framebuffer_rgb(int r, int g, int b) {
    // TODO: support other bpp mode
    return (r << 16) | (g << 8) | b;
}

// should be called after changing either cursor_posx or cursor_posy
static void draw_cursor(bool load_buffer) {
    for(int y = 0; y < font_height; y++) {
        for(int x = 0; x < font_width; x++) {
            if(load_buffer) {
                video_framebuffer_plot_pixel(
                    x + cursor_buffer_posx * font_width,
                    y + cursor_buffer_posy * font_height,
                    cursor_buffer[y * font_width + x]
                );
            }

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

static void load_cursor_buffer() {
    for(int y = 0; y < font_height; y++) {
        for(int x = 0; x < font_width; x++) {
            video_framebuffer_plot_pixel(
                x + cursor_buffer_posx * font_width,
                y + cursor_buffer_posy * font_height,
                cursor_buffer[y * font_width + x]
            );
        }
    }
}

int video_framebuffer_get_cursor() {
    return cursor_posy * text_cols + cursor_posx;
}

void video_framebuffer_set_cursor(int offset) {
    cursor_posy = offset / text_cols;
    cursor_posx = offset % text_cols;
}

void video_framebuffer_cls(int bg) {
    video_framebuffer_fill_rectangle(0, 0, fb_width-1, fb_height-1, bg);
    cursor_posy = 0;
    cursor_posx = 0;
}

static void printc(
    char chr,
    int* _cursor_posx_ptr,
    int* _cursor_posy_ptr,
    int fg,
    int bg,
    bool calculate_mode
) {
    int _cursor_posx = *_cursor_posx_ptr;
    int _cursor_posy = *_cursor_posy_ptr;

    if(chr == '\n') {
        _cursor_posx = 0;
        _cursor_posy++;
    } else if(chr == '\b') {
        _cursor_posx -= 1;
        if(_cursor_posx < 0) {
            _cursor_posx = text_cols - 1;
            _cursor_posy -= 1;
            if(_cursor_posy < 0) _cursor_posy = 0;
        }
        if(!calculate_mode) {
            // manually delete the char
            video_framebuffer_fill_rectangle(
                _cursor_posx * font_width,
                _cursor_posy * font_height,
                (_cursor_posx + 1) * font_width,
                (_cursor_posy + 1) * font_height,
                bg
            );
        }
    } else {
        if(!calculate_mode) {
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
        }
        _cursor_posx++;
    }

    if(_cursor_posx == text_cols) {
        _cursor_posx = 0;
        _cursor_posy++;
    }

    *_cursor_posx_ptr = _cursor_posx;
    *_cursor_posy_ptr = _cursor_posy;
}

void video_framebuffer_printc(char chr, int offset, int fg, int bg, bool move) {
    if(chr == 0) return;

    int _cursor_posx = cursor_posx;
    int _cursor_posy = cursor_posy;
    if(fg < 0) fg = current_fg;
    if(bg < 0) bg = current_bg;

    if(offset >= 0) {
        _cursor_posy = offset / text_cols;
        _cursor_posx = offset % text_cols;
    }

    printc(chr, &_cursor_posx, &_cursor_posy, fg, bg, false);

    if(move) {
        cursor_posx = _cursor_posx;
        cursor_posy = _cursor_posy;
        if(cursor_posy == text_rows) scroll_screen(1);
    }

    draw_cursor(chr == '\n' || chr == '\b');
}

void video_framebuffer_prints(const char* str, int offset, int fg, int bg, bool move) {
    if(!(*str)) return;

    load_cursor_buffer();

    int _cursor_posx = cursor_posx;
    int _cursor_posy = cursor_posy;
    if(fg < 0) fg = current_fg;
    if(bg < 0) bg = current_bg;

    if(offset >= 0) {
        _cursor_posy = offset / text_cols;
        _cursor_posx = offset % text_cols;
    }

    if(move) {
        // precalculate final cursor position for fast scrolling
        int phantomx = _cursor_posx;
        int phantomy = _cursor_posy;
        for(const char* chr = str; *chr != 0; chr++) {
            printc(*chr, &phantomx, &phantomy, fg, bg, true);
        }
        if(phantomy >= text_rows) {
            int scroll_amount = phantomy - text_rows + 1;
            scroll_screen(scroll_amount);

            _cursor_posy -= scroll_amount;
            if(_cursor_posy < 0) _cursor_posy = 0;
        }
    }

    // now print
    for(const char* chr = str; *chr != 0; chr++) {
        if(_cursor_posy >= text_rows) break;
        printc(*chr, &_cursor_posx, &_cursor_posy, fg, bg, false);
    }

    if(move) {
        cursor_posx = _cursor_posx;
        cursor_posy = _cursor_posy;
    }

    draw_cursor(false);
}
