#include "debug.h"

static void print_log_tag(int lt) {
    putchar('[');
    switch(lt) {
        case LT_IF:
            tty_set_attr(WHITE);
            printf("IF");
            break;
        case LT_OK:
            tty_set_attr(LIGHT_GREEN);
            printf("OK");
            break;
        case LT_WN:
            tty_set_attr(LIGHT_BROWN);
            printf("WN");
            break;
        case LT_ER:
            tty_set_attr(LIGHT_RED);
            printf("ER");
            break;
        case LT_CR:
            tty_set_attr(RED);
            printf("CR");
            break;
    }
    tty_set_attr(LIGHT_GREY);
    printf("] ");
}

void print_debug(int log_tag, const char* restrict format, ...) {
    print_log_tag(log_tag);

    va_list arg;
    va_start(arg, format);
    printf(format, va_arg(arg, int));
    va_end(arg);
}
