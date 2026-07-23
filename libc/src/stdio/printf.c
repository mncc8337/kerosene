#include <stdio.h>
#include <unistd.h>

int printf(const char* restrict format, ...) {
    va_list parameters;
    va_start(parameters, format);
    int written = vdprintf(STDOUT_FILENO, format, parameters);
    va_end(parameters);
    return written;
}

