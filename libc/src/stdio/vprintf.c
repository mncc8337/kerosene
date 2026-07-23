#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

int vprintf(const char* restrict format, va_list parameters) {
    return vdprintf(STDOUT_FILENO, format, parameters);
}
