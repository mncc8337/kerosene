#include <stdio.h>
#include <stdarg.h>

int dscanf(int fd, const char *restrict format, ...) {
    va_list parameters;
    va_start(parameters, format);
    int assigned = vdscanf(fd, format, parameters);
    va_end(parameters);
    return assigned;
}
