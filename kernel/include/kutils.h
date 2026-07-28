#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include <process.h>

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

bool validate_user_buffer(const process_t* current_process, const void* buf, size_t size);
bool validate_user_string(const process_t* current_process, const char* str);
