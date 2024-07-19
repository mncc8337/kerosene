#include "debug.h"

void print_log_tag(int lt) {
    putchar('[');
    switch(lt) {
        case LT_INFO:
            tty_set_attr(WHITE);
            printf("IF");
            break;
        case LT_SUCCESS:
            tty_set_attr(LIGHT_GREEN);
            printf("OK");
            break;
        case LT_WARNING:
            tty_set_attr(LIGHT_BROWN);
            printf("WN");
            break;
        case LT_ERROR:
            tty_set_attr(LIGHT_RED);
            printf("ER");
            break;
        case LT_CRITICAL:
            tty_set_attr(RED);
            printf("CR");
            break;
    }
    tty_set_attr(LIGHT_GREY);
    printf("] ");
}
