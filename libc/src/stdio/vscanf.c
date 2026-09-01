#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

int vscanf(const char* restrict format, va_list parameters) {
    return vdscanf(STDIN_FILENO, format, parameters);
}
