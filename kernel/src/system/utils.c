#include <system.h>
#include <video.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static void print_log_tag(int lt) {
    video_printc(
        '[',
        -1,
        video_rgb(VIDEO_LIGHT_GREY),
        video_rgb(VIDEO_BLACK),
        true
    );
    switch(lt) {
        case LT_IF:
            video_prints(
                "INFO",
                -1,
                video_rgb(VIDEO_WHITE),
                video_rgb(VIDEO_BLACK),
                true
            );
            break;
        case LT_OK:
            video_prints(
                " OK ",
                -1,
                video_rgb(VIDEO_LIGHT_GREEN),
                video_rgb(VIDEO_BLACK),
                true
            );
            break;
        case LT_WN:
            video_prints(
                "WARN",
                -1,
                video_rgb(VIDEO_LIGHT_YELLOW),
                video_rgb(VIDEO_BLACK),
                true
            );
            break;
        case LT_ER:
            video_prints(
                "ERR",
                -1,
                video_rgb(VIDEO_LIGHT_RED),
                video_rgb(VIDEO_BLACK),
                true
            );
            break;
        case LT_CR:
            video_prints(
                "CRIT",
                -1,
                video_rgb(VIDEO_RED),
                video_rgb(VIDEO_BLACK),
                true
            );
            break;
    }
    video_prints(
        "] ",
        -1,
        video_rgb(VIDEO_LIGHT_GREY),
        video_rgb(VIDEO_BLACK),
        true
    );
}

static size_t intlen(int num, int radix) {
    if(num == 0) return 1;
    size_t len = 0;
    unsigned n;
    if(radix == 10 && num < 0) {
        len++; // minus sign
        n = -num;
    } else n = (unsigned)num;
    while(n > 0) {
        n /= radix;
        len++;
    }
    return len;
}

static void print(const char* data, size_t length) {
    const unsigned char* bytes = (const unsigned char*)data;
    for(size_t i = 0; i < length; i++) {
        video_printc(bytes[i], -1, -1, -1, true);
    }
}

void kvprintf(const char* restrict format, va_list parameters) {
    while(*format != '\0') {
        if(format[0] != '%' || format[1] == '%') {
            if(format[0] == '%')
                format++;
            size_t amount = 1;
            while(format[amount] && format[amount] != '%')
                amount++;
            print(format, amount);
            format += amount;
            continue;
        }

        const char* format_begun_at = format++;

        if(*format == 'c') {
            format++;
            char c = (char)va_arg(parameters, int);
            print(&c, 1);
        } else if(*format == 's') {
            format++;
            const char* str = va_arg(parameters, const char*);
            if(!str) str = "(null)";
            size_t len = strlen(str);
            print(str, len);
        } else if(*format == 'b') {
            format++;
            int integer = va_arg(parameters, int);
            size_t len = intlen(integer, 2);
            char str[len + 1];
            itoa(integer, str, 2);
            print(str, len);
        } else if(*format == 'd') {
            format++;
            int integer = va_arg(parameters, int);
            size_t len = intlen(integer, 10);
            char str[len + 1];
            itoa(integer, str, 10);
            print(str, len);
        } else if(*format == 'x') {
            format++;
            int integer = va_arg(parameters, int);
            size_t len = intlen(integer, 16);
            char str[len + 1];
            itoa(integer, str, 16);
            print(str, len);
        } else {
            format = format_begun_at;
            size_t len = strlen(format);
            print(format, len);
            format += len;
        }
    }
}

void kprintf(const char* restrict format, ...) {
    va_list arg;
    va_start(arg, format);
    kvprintf(format, arg);
    va_end(arg);
}

void kputchar(const char chr) {
    kprintf("%c", chr);
}

void kputs(const char* str) {
    kprintf("%s\n", str);
}

void kprint_debug(int log_tag, const char* restrict format, ...) {
    print_log_tag(log_tag);

    va_list arg;
    va_start(arg, format);
    kvprintf(format, arg);
    va_end(arg);
}
