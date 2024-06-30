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
    while(timer_ticks < eticks) SYS_SLEEP;
}

char* row_str;
char* col_str;
void print_typed_char(struct key k) {
    if(k.released) return;

    int _row = k.keycode >> 4;
    int _col = k.keycode & 0xf;

    print_string("     ", MAX_COLS * 2 - 5, 0, false);
    print_string(to_string(_row), MAX_COLS * 2 - 5, 0, false);
    print_string(to_string(_col), MAX_COLS * 2 - 2, 0, false);

    unsigned char mapped = key_map(k.keycode, KBL_US);

    if(mapped == '\b') {
        set_cursor(get_cursor() - 1); // move back
        print_char(' ', -1, 0, false); // delete printed char
    }
    else {
        print_char(mapped, -1, 0, true);
    }
}

void main() {
    // cls(0x0f);

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

    print_string("hello\n", -1, CYAN, true);
    print_string("it's a running ", -1, MAGENTA, true);
    print_string("kernel!\n", -1, RED, true);

    kbd_init();

    char* prompt = "[kernel$sos]$ ";

    char c;
    char inp[512];
    int token_pos[512];
    int token_count = 0;

    char* command;

    while(true) {
        get_string(inp, '\n', print_typed_char);
        print_char('\n', -1, 0, true);
    }
}
