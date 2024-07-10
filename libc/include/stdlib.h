#pragma once

#include <sys/cdefs.h>

#define EOF (-1)

__attribute__((__noreturn__))
void abort(void);

char* itoa(int num, char* buff, int radix);
