#pragma once

#include <stdarg.h>

enum LOG_TAG{
    LT_IF,
    LT_OK,
    LT_WN,
    LT_ER,
    LT_CR
};

void kvprintf(const char* restrict format, va_list parameters);
void kprintf(const char* restrict format, ...);
void kputchar(const char chr);
void kputs(const char* str);
void kprint_debug(int log_tag, const char* restrict format, ...);
