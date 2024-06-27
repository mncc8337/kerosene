#include "system.h"
#include "driver/video.h"

const unsigned int TIMER_PHASE = 1;

volatile unsigned int timer_ticks = 0;
char* sec_msg = " ticks passed";
void runtime_timer_handler() {
    timer_ticks++;

    char* tm = to_string(timer_ticks/TIMER_PHASE);

    print_string(tm, MAX_COLS - string_len(tm) - 13, 0x04);
    print_string(sec_msg, MAX_COLS - 13, 0x04);
}
void timer_wait(unsigned int ticks) {
    unsigned long eticks;

    eticks = timer_ticks + ticks;
    while(timer_ticks < eticks)
        asm volatile("sti; hlt; cli"); // sleep mode
}

void main() {
    gdt_init();
    idt_init();
    irq_init();

    // start receiving interrupts
    // after initialize all interrupt handlers
    asm volatile("sti");

    pit_timer_phase(TIMER_PHASE);
    irq_install_handler(0, runtime_timer_handler);

    enable_cursor(13, 14);

    cls(0x0f);

    print_string("hello", -1, 0x05);
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
