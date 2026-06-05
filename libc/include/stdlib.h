#pragma once

#include <sys/cdefs.h>

#define EOF (-1)

__attribute__((__noreturn__))
void abort(void);

__attribute__((__noreturn__))
void exit(int exit_code);

char* itoa(int num, char* buff, int radix);
int atoi(char* buff);
