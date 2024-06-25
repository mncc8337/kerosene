#include "video.h"

void main() {
    init_video();
    enable_cursor(13, 14);

    cls(0x0f);

    print_string("hello", 0, 0x05);
    print_string("omg it's la running kernel!", get_offset(1, 2), 0x0f);

    char wth[] = "wut duh hell ";

    for(int i = MAX_COLS * 2; i < MAX_COLS * MAX_ROWS; i += 13) {
        unsigned char bg = i % 16;
        unsigned char fg = (i + 8) % 16;
        print_string(wth, i, bg * 16 + fg);
    }

    // hang
    while(1);
}
