#pragma once

#include "stdarg.h"

#include "tty.h"
#include "stdio.h"

enum LOG_TAG{
    LT_IF,
    LT_OK,
    LT_WN,
    LT_ER,
    LT_CR
};

void print_debug(int log_tag, const char* restrict format, ...);
