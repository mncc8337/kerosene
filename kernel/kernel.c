#include "system.h"
#include "driver/video.h"
#include "driver/kbd.h"
#include "driver/pit.h"

const unsigned int TIMER_PHASE = 100;

volatile unsigned int timer_ticks = 0;
char* sec_msg = "seconds elapsed";
char* tm;
void timer_handler() {
    timer_ticks++;

    tm = to_string(timer_ticks/TIMER_PHASE);

    print_string(tm, MAX_COLS - string_len(tm) - 17, 0x04, false);
    print_string(sec_msg, MAX_COLS - 15, 0x04, false);
}
void timer_wait(unsigned int ticks) {
    unsigned long eticks;

    eticks = timer_ticks + ticks;
    while(timer_ticks < eticks)
        asm volatile("sti; hlt; cli"); // sleep mode
}

void print_typed_char(char chr) {
    if(chr == '\b') {
        set_cursor(get_cursor() - 1); // move back
        print_char(' ', -1, 0, false); // delete printed char
    }
    else {
        print_char(chr, -1, 0, true);
    }
}

void main() {
    gdt_init();
    idt_init();
    irq_init();

    // start receiving interrupts
    // after initialize all interrupt handlers
    asm volatile("sti");

    pit_timer_phase(TIMER_PHASE);
    irq_install_handler(0, timer_handler);

    // make cursor slimmer
    enable_cursor(13, 14);

    cls(0x0f);

    print_string("hello", -1, 0x05, false);
    print_string("omg it's la running kernel!\n", get_offset(1, 2), 0x0f, true);
    print_string("tealdf\n", -1, 0x0f, true);
    print_string("bruh\n", -1, 0x0f, true);

    char c;
    char* inp;
    while(true) {
        get_string(inp, '\n', print_typed_char);
        print_char('\n', -1, 0, true);
    }
}
