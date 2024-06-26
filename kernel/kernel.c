#include "system.h"
#include "driver/video.h"

void main() {
    // init video first so we can print error messages
    video_init();

    gdt_init();
    idt_init();
    irq_init();

    // start receiving interrupts
    // after initialize all interrupt handlers
    asm volatile("sti");

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

    set_cursor(get_offset(1, 0));

    while(true);
}
