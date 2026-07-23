#pragma once

#include <sys/cdefs.h>
#include <stdarg.h>
#include <bits/seek.h>

#define EOF (-1)

int vdprintf(int fd, const char* restrict format, va_list parameters);
int dprintf(int fd, const char* restrict format, ...);
int vprintf(const char* restrict format, va_list parameters);
int printf(const char* restrict format, ...);
int putchar(int ic);
int puts(const char* string);
