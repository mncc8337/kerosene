#include "string.h"

char* strrev(char* str) {
    unsigned int len = strlen(str);

    for(unsigned int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }

    return str;
}
