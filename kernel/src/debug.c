#include "debug.h"
#include "video.h"
#include "stdio.h"

static void print_log_tag(int lt) {
    putchar('[');
    switch(lt) {
        case LT_IF:
            video_set_attr(TTY_WHITE);
            printf("IF");
            break;
        case LT_OK:
            video_set_attr(TTY_LIGHT_GREEN);
            printf("OK");
            break;
        case LT_WN:
            video_set_attr(TTY_LIGHT_BROWN);
            printf("WN");
            break;
        case LT_ER:
            video_set_attr(TTY_LIGHT_RED);
            printf("ER");
            break;
        case LT_CR:
            video_set_attr(TTY_RED);
            printf("CR");
            break;
    }
    video_set_attr(TTY_LIGHT_GREY);
    printf("] ");
}

void print_debug(int log_tag, const char* restrict format, ...) {
    print_log_tag(log_tag);

    va_list arg;
    va_start(arg, format);
    printf(format, va_arg(arg, int));
    va_end(arg);
}
