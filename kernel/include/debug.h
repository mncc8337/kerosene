#pragma once

#include "tty.h"
#include "stdio.h"

enum LOG_TAG{
    LT_INFO,
    LT_SUCCESS,
    LT_WARNING,
    LT_ERROR,
    LT_CRITICAL
};

void print_log_tag(int lt);
