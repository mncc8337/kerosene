#include "string.h"

bool strcmp(char* str1, char* str2) {
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
