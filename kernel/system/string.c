#include "system.h"

int string_len(const char* str) {
    int len = 0;
    while(str[len] != '\0')
        len++;
    len--;
    return len;
}

char* to_string(int num) {
    char* res;
    unsigned int cnt = 0;

    bool _signed = num < 0;
    if(_signed) {
        num *= -1;
        res[0] = '-';
        cnt++;
    }

    while(num > 0) {
        res[cnt] = num % 10 + '0';
        num /= 10;
        cnt++;
    }
    res[cnt] = '\0';

    // reverse the string
    char tmp ;
    for(int i = _signed; i < (cnt - _signed)/2; i++) {
        tmp = res[i];
        res[i] = res[cnt - i - 1 + _signed];
        res[cnt - i - 1 + _signed] = tmp;
    }

    return res;
}
