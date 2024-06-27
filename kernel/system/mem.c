#include "system.h"

unsigned char* memcpy(unsigned char* dest, unsigned char* src, int cnt) {
    while(cnt) {
        cnt--;
        dest[cnt] = src[cnt];
    }
    return dest;
}

unsigned char* memset(unsigned char* dest, unsigned char val, int cnt) {
    while(cnt) {
        cnt--;
        dest[cnt] = val;
    }
    return dest;
}
unsigned short* memsetw(unsigned short* dest, unsigned short val, int cnt) {
    while(cnt) {
        cnt--;
        dest[cnt] = val;
    }
    return dest;
}
