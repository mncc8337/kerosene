#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

int scanf(const char *restrict format, ...) {
    va_list parameters;
    va_start(parameters, format);
    int assigned = vdscanf(STDIN_FILENO, format, parameters);
    va_end(parameters);
    return assigned;
}
