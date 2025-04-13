#include "string.h"

char* strtok(char* str, const char* delimiters, char** old) {
    if(str != NULL) *old = str;
    else if(**old == '\0') return NULL;

    // skip leading delimiters
    while((*old)[0] != '\0') {
        bool delimiters_exist = false;
        for(int i = 0; delimiters[i] != '\0'; i++) {
            if((*old)[0] == delimiters[i]) {
                delimiters_exist = true;
                break;
            }
        }
        if(delimiters_exist) (*old)++;
        else break;
    }
    // the string is full of delimiters
    if((*old)[0] == '\0') return NULL;

    int pos = 0;
    while((*old)[pos] != '\0') {
        for(int i = 0; delimiters[i] != '\0'; i++) {
            if(delimiters[i] != (*old)[pos]) continue;
            (*old)[pos] = '\0';
            *old += pos + 1;
            return *old - pos - 1;
        }
        pos++;
    }

    // reached the end of the string
    *old += pos;
    return *old - pos;
}
