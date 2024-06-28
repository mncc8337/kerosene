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

void print_typed_char(struct key k) {
    if(k.released) return;

    if(k.mapped == '\b') {
        set_cursor(get_cursor() - 1); // move back
        print_char(' ', -1, 0, false); // delete printed char
    }
    else {
        print_char(k.mapped, -1, 0, true);
    }
}

void main() {
    cls(0x0f);

    bool a20_enabled = enable_a20();
    if(a20_enabled) print_string("A20 enabled\n", -1, 0, true);
    else print_string("failed to enable A20, giving up\n", -1, 0, true);

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
        print_string(prompt, -1, GREEN, true);
        get_string(inp, '\n', print_typed_char);
        print_char('\n', -1, 0, true);

        if(string_len(inp) == 0) continue;

        tokenize(inp, ' ', token_pos, &token_count);
        command = substr(inp, token_pos[0], token_pos[1]+1);

        if(string_cmp(command, "echo")) {
            print_string(inp + token_pos[2], -1, 0, true);
        }
        else if(string_cmp(command, "clear")) {
            cls(0x0f);
        }
        else {
            print_string("unknown command \"", -1, WHITE, true);
            print_string(inp, -1, RED, true);
            print_char('"', -1, WHITE, true);
        }
        print_char('\n', -1, 0, true);
    }
}
