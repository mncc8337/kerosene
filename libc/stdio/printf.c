#include "stdio.h"
#include "stdlib.h"
#include "limits.h"
#include "stdbool.h"
#include "stdarg.h"
#include "string.h"

static bool print(const char* data, size_t length) {
    const unsigned char* bytes = (const unsigned char*)data;
    for(size_t i = 0; i < length; i++)
        if(putchar(bytes[i]) == EOF)
            return false;
    return true;
}
static size_t intlen(int num, int radix) {
    if(num == 0) return 1;
    size_t len = 0;
    if(radix == 10 && num < 0) {
        len++; // minus sign
        num *= -1;
    }
    while(num > 0) {
        num /= radix;
        len++;
    }
    return len;
}

int printf(const char* restrict format, ...) {
    va_list parameters;
    va_start(parameters, format);

    int written = 0;

    while(*format != '\0') {
        size_t maxrem = INT_MAX - written;

        if(format[0] != '%' || format[1] == '%') {
            if(format[0] == '%')
                format++;
            size_t amount = 1;
            while(format[amount] && format[amount] != '%')
                amount++;
            if(maxrem < amount) {
                // TODO: set errno to ERR_OVERFLOW
                return -1;
            }
            if(!print(format, amount))
                return -1;
            format += amount;
            written += amount;
            continue;
        }

        const char* format_begun_at = format++;

        if(*format == 'c') {
            format++;
            char c = (char) va_arg(parameters, int /* char promotes to int */);
            if(!maxrem) {
                // TODO: set errno to ERR_OVERFLOW
                return -1;
            }
            if(!print(&c, sizeof(c)))
                return -1;
            written++;
        }
        else if(*format == 's') {
            format++;
            const char* str = va_arg(parameters, const char*);
            size_t len = strlen(str);
            if(maxrem < len) {
                // TODO: set errno to ERR_OVERFLOW
                return -1;
            }
            if(!print(str, len))
                return -1;
            written += len;
        }
        else if(*format == 'b') {
            format++;
            int integer = va_arg(parameters, int);
            size_t len = intlen(integer, 2);
            char str[len + 1]; itoa(integer, str, 2);
            if(maxrem < len) {
                // TODO: set errno to ERR_OVERFLOW
                return -1;
            }
            if(!print(str, len))
                return -1;
            written += len;
        }
        else if(*format == 'o') {
            format++;
            int integer = va_arg(parameters, int);
            size_t len = intlen(integer, 8);
            char str[len + 1]; itoa(integer, str, 8);
            if(maxrem < len) {
                // TODO: set errno to ERR_OVERFLOW
                return -1;
            }
            if(!print(str, len))
                return -1;
            written += len;
        }
        else if(*format == 'd') {
            format++;
            int integer = va_arg(parameters, int);
            size_t len = intlen(integer, 10);
            char str[len + 1]; itoa(integer, str, 10);
            if(maxrem < len) {
                // TODO: set errno to ERR_OVERFLOW
                return -1;
            }
            if(!print(str, len))
                return -1;
            written += len;
        }
        else if(*format == 'x') {
            format++;
            int integer = va_arg(parameters, int);
            size_t len = intlen(integer, 16);
            char str[len + 1]; itoa(integer, str, 16);
            if(maxrem < len) {
                // TODO: set errno to ERR_OVERFLOW
                return -1;
            }
            if(!print(str, len))
                return -1;
            written += len;
        }
        else {
            format = format_begun_at;
            size_t len = strlen(format);
            if(maxrem < len) {
                // TODO: set errno to ERR_OVERFLOW
                return -1;
            }
            if(!print(format, len))
                return -1;
            written += len;
            format += len;
        }
    }

    va_end(parameters);
    return written;
}
