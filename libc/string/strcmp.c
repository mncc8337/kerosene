#include "string.h"

bool strcmp(const char* str1, const char* str2) {
    int t = 0;
    while(str1[t] == str2[t]) {
        if(str1[t] == '\0' || str2[t] == '\0')
            break;
        t++;
    }
    if(str1[t] == '\0' && str2[t] == '\0')
        return true;
    return false;
}

// TODO: implement ctype.h
static char toupper(char a) {
    if(a >= 0x61 && a <= 0x7a) a -= 0x20;
    return a;
}

bool strcmp_case_insensitive(const char* str1, const char* str2) {
    int t = 0;
    while(str1[t] == str2[t] || toupper(str1[t]) == str2[t] || str1[t] == toupper(str2[t])) {
        if(str1[t] == '\0' || str2[t] == '\0')
            break;
        t++;
    }
    if(str1[t] == '\0' && str2[t] == '\0')
        return true;
    return false;
}
