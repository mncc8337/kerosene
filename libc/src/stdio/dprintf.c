#include <stdio.h>

int dprintf(int fd, const char *restrict format, ...) {
    va_list parameters;
    va_start(parameters, format);
    int written = vdprintf(fd, format, parameters);
    va_end(parameters);
    return written;
}
