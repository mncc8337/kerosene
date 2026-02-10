#include <stdlib.h>

int atoi(char* buff) {
    int ret = 0;

    char* p = buff;
    while(*p != '\0') {
        ret = ret * 10 + (*p) - '0';
        p++;
    }

    return ret;
}
