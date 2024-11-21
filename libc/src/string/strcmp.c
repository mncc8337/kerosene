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
static char toupper(char chr) {
    bool is_lower = (chr >= 'a' && chr <= 'z');
    chr += 32 * is_lower;
    return chr;
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
