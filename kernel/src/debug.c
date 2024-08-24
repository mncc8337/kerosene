#include "debug.h"
#include "video.h"
#include "stdio.h"

static void print_log_tag(int lt) {
    putchar('[');
    switch(lt) {
        case LT_IF:
            video_set_attr(video_rgb(VIDEO_WHITE), video_rgb(VIDEO_BLACK));
            printf("IF");
            break;
        case LT_OK:
            video_set_attr(video_rgb(VIDEO_LIGHT_GREEN), video_rgb(VIDEO_BLACK));
            printf("OK");
            break;
        case LT_WN:
            video_set_attr(video_rgb(VIDEO_LIGHT_YELLOW), video_rgb(VIDEO_BLACK));
            printf("WN");
            break;
        case LT_ER:
            video_set_attr(video_rgb(VIDEO_LIGHT_RED), video_rgb(VIDEO_BLACK));
            printf("ER");
            break;
        case LT_CR:
            video_set_attr(video_rgb(VIDEO_RED), video_rgb(VIDEO_BLACK));
            printf("CR");
            break;
    }
    video_set_attr(video_rgb(VIDEO_LIGHT_GREY), video_rgb(VIDEO_BLACK));
    printf("] ");
}

void print_debug(int log_tag, const char* restrict format, ...) {
    print_log_tag(log_tag);

    va_list arg;
    va_start(arg, format);
    printf(format, va_arg(arg, int));
    va_end(arg);
}
