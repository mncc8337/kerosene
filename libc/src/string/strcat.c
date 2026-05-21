#include <string.h>

char* strcat(char* restrict dst, const char* restrict src) {
    char* d = dst;
    while(*d) d++;
    while((*d++ = *src++));
    return dst;
}
