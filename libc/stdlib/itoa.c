#include "stdlib.h"
#include "string.h"

char* itoa(int num, char* buff, int radix) {
    int i;
    unsigned int n;

    if(num == 0) {
        buff[0] = '0';
        buff[1] = '\0';
        return buff;
    }

    int sign = (radix == 10 && num < 0);    
    if(sign) n = -num;
    else n = (unsigned)num;

    int cnt = 0;
    while(n) {
        i = n % radix;
        n /= radix;
        if(i < 10) buff[cnt] = i + '0';
        else buff[cnt] = i + 'a' - 10;
        cnt++;
    }

    if(sign) {
        buff[cnt] = '-';
        cnt++;
    }
    buff[cnt] = '\0';

    strrev(buff);

    return buff;
}
