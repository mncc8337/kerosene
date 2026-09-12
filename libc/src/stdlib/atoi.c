#include <stdlib.h>

int atoi(char* buff) {
    int ret = 0;
    int sign = 1;
    char* p = buff;

    if(*p == '-') {
        sign = -1;
        p++;
    }

    while(*p != '\0') {
        ret = ret * 10 + (*p) - '0';
        p++;
    }

    return ret * sign;
}
