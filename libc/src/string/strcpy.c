#include <string.h>

char* strcpy(char* restrict dst, const char* restrict src) {
    char* d = dst;
    while((*d++ = *src++));
    return dst;
}
